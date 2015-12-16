#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

#include <cstring>

#include <iostream>
#include <sstream>
#include <fstream>

pid_t pid;
pid_t cpid;

char *err_proc;
char *exec_proc;
char *pidfile_path_task;
char *pidfile_path_monitor;
int max_tries;
int seconds;
bool first_try = true;

struct pstat {
    long unsigned int utime_ticks, stime_ticks, vsize, rss, cpu_total_time;
    long int cutime_ticks, cstime_ticks;
};

void exitAsExpected(int sig) {
    kill (pid, sig);
}

void parseStats(struct pstat* result) {
    char pid_s[20];
    snprintf(pid_s, sizeof(pid_s), "%d", cpid);
    char stat_filepath[30] = "/proc/"; strncat(stat_filepath, pid_s,
    sizeof(stat_filepath) - strlen(stat_filepath) -1);
    strncat(stat_filepath, "/stat", sizeof(stat_filepath) -
    strlen(stat_filepath) -1);

    FILE *fpstat = fopen(stat_filepath, "r");
    if (fpstat == NULL)
        std::cerr << "Process not found!" << std::endl;

    FILE *fstat = fopen("/proc/stat", "r");
    if (fstat == NULL)
        std::cerr << "Error in opening system stats!" << std::endl;

    bzero(result, sizeof(struct pstat));
    long int rss;
    if (fscanf(fpstat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
                "%lu %ld %ld %*d %*d %*d %*d %*u %lu %ld",
                &result->utime_ticks, &result->stime_ticks,
                &result->cutime_ticks, &result->cstime_ticks, &result->vsize,
                &rss) == EOF) {
        fclose(fpstat);
        std::cerr << "Error while parsing process info" << std::endl;
        exit(EXIT_FAILURE);
    }
    fclose(fpstat);
    result->rss = rss * getpagesize();

    long unsigned int cpu_time[10];
    bzero(cpu_time, sizeof(cpu_time));
    if (fscanf(fstat, "%*s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                &cpu_time[0], &cpu_time[1], &cpu_time[2], &cpu_time[3],
                &cpu_time[4], &cpu_time[5], &cpu_time[6], &cpu_time[7],
                &cpu_time[8], &cpu_time[9]) == EOF) {
        fclose(fstat);
        std::cerr << "Error while parsing system info" << std::endl;
        exit(EXIT_FAILURE);
    }
    fclose(fstat);

    for (int i = 0; i < 10; i++)
        result->cpu_total_time += cpu_time[i];
}

void usageCPU(const struct pstat* curr, const struct pstat* prev, double* userUsage, double* systemUsage) {

     const long unsigned int totalTimeDiff = curr->cpu_total_time - prev->cpu_total_time;

     *userUsage = 100 * (((curr->utime_ticks + curr->cutime_ticks) - (prev->utime_ticks + prev->cutime_ticks)) / ((double) totalTimeDiff));

     *systemUsage = 100 * (((curr->stime_ticks + curr->cstime_ticks) - (prev->stime_ticks + prev->cstime_ticks)) / ((double) totalTimeDiff));
}

void usageCPU2(const struct pstat* curr, const struct pstat* prev, double* userUsage, double* systemUsage) {
     const long unsigned int totalTimeDiff = curr->cpu_total_time - prev->cpu_total_time;

     *userUsage = ((curr->utime_ticks + curr->cutime_ticks) - (prev->utime_ticks + prev->cutime_ticks));

     *systemUsage = ((curr->stime_ticks + curr->cstime_ticks) - (prev->stime_ticks + prev->cstime_ticks));
}


void * processStats(void *) {
    while (1) {
        pstat prev, curr;
        double user, system;

        parseStats(&prev);
        usleep(20000);
        parseStats(&curr);

        usageCPU(&curr, &prev, &user, &system);

        std::cout << "User CPU Usage: " << user << "     System CPU Usage: " << system << std::endl;
        sleep(seconds);
    }
}

void processMonitor() {
    int status;

    switch (pid = fork()) {
        case -1:
            // error occured while forking
            exit(EXIT_FAILURE);
        case 0:
            // this is the child
            execl("/bin/sh", "sh", "-c", exec_proc, NULL);
            exit(EXIT_FAILURE);
        default:
            // parent proc

            // searches for the exec process (I have no idea why this isn't equal to fork())
            char line[100];
            char pname[100] = "pidof -s ";
            strcat(pname, exec_proc);
            FILE *cmd = popen(pname, "r");
            fgets(line, 100, cmd);
            cpid = strtoul(line, NULL, 10);
            pclose(cmd);

            if (first_try) {
                first_try = false;
                pthread_t t1;
                pthread_create(&t1, NULL, &processStats, NULL);
            }
            waitpid(pid, &status, 0);

            if (err_proc)
                system(err_proc);

            if (max_tries) {
                max_tries--;
                // write to log file here
                time_t now = time(0);
                tm* localtm = localtime(&now);
                tm* gmtm = gmtime(&now);

                std::ofstream myfile;
                myfile.open("crashlog.txt", std::ofstream::out | std::ofstream::app);
                myfile << "Process " << exec_proc << " failed with PID " << cpid << std::endl;
                myfile << "Time of crash:    " << asctime(gmtm) << std::endl << std::endl;
                myfile.close();
                if (max_tries == 0)
                    exit(EXIT_SUCCESS);
            }
            std:: cout << max_tries << " restarts remaining" << std::endl;
            processMonitor();
    }
}

int main(int argc, char ** argv) {
    int c;

    if (argc == 1) {
        std::cerr << "Please enter arguments, use -h for more info on usage" << std::endl;
        exit(EXIT_FAILURE);
    }

    while ((c = getopt(argc, argv, ":ht:i:l:")) != -1) {
        switch (c) {
            case 't':
                std::cout << "Received task to monitor" << std::endl;
                exec_proc = optarg;
                std::cout << exec_proc << std::endl;
                break;
            case 'i':
                std::cout << "Received interval (seconds)" <<std::endl;
                seconds = atoi(optarg);
                if (seconds <= 0) {
                    std::cerr << "Please enter a positive value (>1) for -i" << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;

            case 'l':
                std::cout << "Received restart limit" << std::endl;
                max_tries = atoi(optarg); // convert char * to int
                break;
            case ':':
                std::cerr << "Missing args" << std::endl;
                exit(EXIT_FAILURE);
                break;
            case '?':
                std::cerr << "Unrecognized option" <<std::endl;
                exit(EXIT_FAILURE);
            case 'h':
                std::cout << "usage: monitor [options] [process]" << std::endl;
                std::cout << "-t <dir>" << std::endl;
                std::cout << "-i <integer>" << std::endl;
                std::cout << "-l <integer>" << std::endl;
                std::cout << "-h " << std::endl;;
                exit(EXIT_SUCCESS);
        }
    }
    signal(SIGTERM, &exitAsExpected);
    signal(SIGQUIT, &exitAsExpected);

    processMonitor();
}
