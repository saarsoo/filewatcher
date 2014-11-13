Filewatcher
===========

Filewatcher built in C. The watcher will report any changes in the folder specified.

Support for verbose mode!

Example output:
```
Checking input... OK!
Validating input... OK!
Initializing Watchers... OK!
Initializing Watcher Thread... OK!
Running...

[10:28:43][CREATED][FILE] filename
[10:28:48][CREATED][DIRECTORY] directory
[10:28:50][DELETED][FILE] filename
[10:28:54][DELETED][DIRECTORY] directory
[10:29:09][CREATED][FILE] file
[10:29:09][MODIFIED][FILE] file
[10:29:21][RENAMED][FILE] file -> newfile
```
