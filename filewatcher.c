#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "textcolor.h"

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef enum { false, true } bool;

struct watcher {
	int wd;
	int fd;
	struct watcher *next;
};

static struct watcher *root;
static bool running = true;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *watchers();
void intHandler(int);

int main(int argc, char *argv[]){
	system("clear");

	print(SUCCESS, "Checking input... ");
	if (argc == 1) {
		print(ERROR, "You must specify which folders to watch!\n");
		return 1;
	} else {
		print(SUCCESS, "OK!\n");
	}

	print(SUCCESS, "Validating input... ");
	int i, err;
	struct stat s;
	for (i = 1; i < argc; i++){
		err = stat(argv[i], &s);
		if (err == -1) {
			if (ENOENT == errno) {
        print(ERROR, "Directory \"%s\" doesn't exist!\n", argv[i]);
				return 2;
			}
		} else {
			if (!S_ISDIR(s.st_mode)) {
        print(ERROR, "\"%s\" is not a directory!\n", argv[i]);
				//return 3;
			}
		}
	}

  print(SUCCESS, "OK!\n");

	print(SUCCESS, "Initializing Watchers... ");

	root = (struct watcher *)malloc(sizeof(struct watcher));

	root->fd = inotify_init();

	if (root->fd < 0) {
		print(ERROR, "Failed to initialize watcher.\n");
		return 4;
	}

	root->wd = inotify_add_watch(root->fd, argv[1], IN_ALL_EVENTS);

	if (root->wd < 0) {
		print(ERROR, "Failed to initialize watcher.\n");
		return 5;
	}

	print(SUCCESS, "OK!\n");

	print(SUCCESS, "Initializing Watcher Thread... ");
	
	pthread_t w_pt;

	if (pthread_create(&w_pt, NULL, watchers, NULL)) {
		fprintf(stderr, "Failed creating watcher thread!");
	}
	
	print(SUCCESS, "OK!\n");

	print(SUCCESS, "Running...\n\n");

	signal(SIGINT, intHandler);

	while(running) {
		pthread_cond_wait(&cond, &mutex);
	}

	print(OTHER, "\nCleaning up... ");

	(void)close(root->fd);

  if (pthread_join(w_pt, NULL)) {
          print(INFO, "Thread had already exited (?)");
          return 6;
  }

	print(OTHER, "Done!\n");

	return 0;
}

void *watchers(){
	char buffer[BUF_LEN];
	int length, i;

	while(running) {
		length = read(root->fd, buffer, BUF_LEN);
		//print(BLUE, "Finished read, translating\n");

		if (length <= 0) {
			printf("No length");
			break;
		}

		i = 0;

		while (i < length) {
			//print(SUCCESS, "Reading event...");
			struct inotify_event *event = (struct inotify_event *)&buffer[i];
			if (event->len) {
        char *msg;
        //print(INFO, "Event type: (%d)\n", event->mask);

        int filter = (event->mask & IN_CREATE || event->mask & IN_MODIFY || event->mask & IN_DELETE || event->mask & IN_DELETE_SELF);
       
        if (filter || event->mask & IN_MOVED_FROM) { 
          int len;

          time_t t = time(NULL);
          struct tm *p = localtime(&t);

          char s[11];
          strftime(s, 11, "%H:%M:%S", p);
          print(WHITE, "[");
          print(BLUE, "%s", s);
          print(WHITE, "]");
        }

        if (filter || event->mask & IN_MOVED_FROM) {
          print(WHITE, "[");
        }

				if (event->mask & IN_CREATE) {
          print(GREEN, "CREATED", event->name);
				} else if (event->mask & IN_MODIFY) {
          print(INFO, "MODIFIED");
				} else if (event->mask & IN_DELETE) {
          print(RED, "DELETED", event->name);
				} else if (event->mask & IN_DELETE_SELF) {
					printf("Base directory \"%s\" was deleted.\n", event->name);
				} else if (event->mask & IN_MOVED_FROM) {
          print(INFO, "RENAMED");
        } else if (event->mask & IN_MOVED_TO) {
          print(WHITE, " -> %s", event->name);
        }

        if (filter || event->mask & IN_MOVED_FROM) {
          print(WHITE, "]");
        }

        if (filter || event->mask & IN_MOVED_FROM) {
          print(WHITE, "[");
          if (event->mask & IN_ISDIR) {
            print(BLUE, "DIRECTORY");
          } else {
            print(PURPLE, "FILE");
          }
          print(WHITE, "]");
        }

        if (filter || event->mask & IN_MOVED_FROM) {
          print(WHITE, " %s", event->name);
        }

        if (filter || event->mask & IN_MOVED_TO) {
          printf("\n");
        }

			}	
			
			i += EVENT_SIZE + event->len;
		}
    //print(BLUE, "End of translation\n");
	}

	pthread_cond_signal(&cond);
}

void intHandler(int dummy) {
	print(OTHER, "\nKill signal sent, exiting...");
	running = false;
	(void)inotify_rm_watch(root->fd, root->wd);
}
