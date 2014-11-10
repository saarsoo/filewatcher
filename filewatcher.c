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
static char *command = "";

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *watchers();
void intHandler(int);
void copy_string(char *, char *);

int main(int argc, char **argv){
  int opt;
  const char *path = "";

  if (argc == 1) {
    print(ERROR, "You must specify which folder to watch!\n");
    return 1;
  }

  path = argv[1];

  struct stat s;

  if (stat(path, &s) == -1) {
    if (ENOENT == errno) {
      print(ERROR, "Directory \"%s\" doesn't exist!\n", path);
      return 2;
    }
  } else {
    if (!S_ISDIR(s.st_mode)) {
      print(ERROR, "\"%s\" is not a directory!\n", path);
      return 3;
    }
  }

  while ((opt = getopt(argc, argv, "c:")) != -1) {
    switch (opt) {
      case 'c':
        command = optarg;
        break;
      case '?':
        path = optarg;
        break;
    }
  }

  if (command == "") {
    print(INFO, "No command specified, running in log mode.\n");
  }


  root = (struct watcher *)malloc(sizeof(struct watcher));

  root->fd = inotify_init();

  if (root->fd < 0) {
    print(ERROR, "Failed to initialize watcher.\n");
    return 4;
  }

  root->wd = inotify_add_watch(root->fd, path, IN_ALL_EVENTS);

  if (root->wd < 0) {
    print(ERROR, "Failed to initialize watcher.\n");
    return 5;
  }

  pthread_t w_pt;

  if (pthread_create(&w_pt, NULL, watchers, NULL)) {
    fprintf(stderr, "Failed creating watcher thread!");
  }

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

    if (length <= 0) {
      printf("No length");
      break;
    }

    i = 0;

    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];
      if (event->len) {
        char *msg;
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
            print(PURPLE, "DIRECTORY");
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

        if (command != "" && (filter || event->mask & IN_MOVED_TO)) {
          char *result = strstr(command, "\%file");
          int position = result - command;

          char before[strlen(command)];

          copy_string(before, command);
          before[position] = '\0';

          char *c;
          asprintf(&c, "%s%s%s", before, event->name, &result[5]);
          system(c);
        }
      }	

      i += EVENT_SIZE + event->len;
    }
  }

  pthread_cond_signal(&cond);
}

void copy_string(char *target, char *source) {
  while (*source) {
    *target = *source;
    source++;
    target++;
  }

  *target = '\0';
}

void intHandler(int dummy) {
  print(OTHER, "\nKill signal sent, exiting...");
  running = false;
  (void)inotify_rm_watch(root->fd, root->wd);
}
