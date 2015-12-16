# Process Monitor
## Frank Cao


This is a program that will guard a process given a number of "lives" for the process. After the number of lives has run out, the process will be terminated for good (before that, the process will be restarted so long as it still has more lives). During the lifetime of the guard, the monitor will print out CPU usage of the process.

usage: monitor [options] [process]
-t <dir> *specify the process/task name REQUIRED*
-i <dir> *specify the interval for process info REQUIRED*
-l <dir> *specify the number of "live" for the process/task REQUIRED*
-h *specifies the usage of the program*

Uses getopt for parsing console arguments and uses /proc/[PID]/stat to parse process/task stats. When the process crashes, the program will add an entry to the crash log. A sample task (node js server) is included.

