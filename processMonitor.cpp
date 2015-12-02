#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


bool file_exists(std::string & fname) {
    struct stat info;
    if (stat(fname.c_str(), &info) == 0) {
        return !(info.st_mode & S_IFDIR);
    } else {
         return true;
    }
}

void recreate_process(std::string & fname, std::string & args) {
    pid_t child = fork();
    if (child == -1)
        exit(1);
    if (child == 0) {
         char ** argv = new char *[args.size() + 2];
         for (size_t i = 0; i < args.size(); i++)
            argv[i] = const_cast<char *>(args[i].c_str());
         argv[args.size()] = NULL;
         if (execve(fname.c_str(), argv, environ) == -1)
             exit(1);
    }
}

bool find_process (std::string & process, std::string & output_fname) {
    if (file_exists(process)) {
        output_fname = process;
        return true;
    }

    const char * path = getenv("PATH");
    const char * path_begin = path;
    const char * path_end = path + strlen(path);

    std::string local_fname;

    while (path_begin < path_end) {
         const char * limit = strstr(path_begin, ":");

         if (!limit)
             limit = path_end;
         local_fname.reserve(std::max(local_fname.capacity(),(limit - path_begin) + process.length() + 1));
         local_fname.append(1, '/');
         local_fname.append(process);

         if (file_exists(local_fname)) {
              output_fname = local_fname;
              return true;
         }

         path_begin = (*limit == ':' ? limit + 1 : limit);
    }
    return false;
}

int main (int argc, const char ** argv) {
    std::string process;
    std::string args[5]; // fairly sure I should use vectors

    // also need to write a parser for strings, maybe use a library if args are to be supported
    // currently using cin

    std::string process_fname;

    std::cout << "Please enter a file name: ";
    std::cin >> process_fname;

    if (!find_process(args[0], process_fname)) {
        std::cout << args[0] << " process not found" << std::endl;
        return 1;
    }

    do {
        // does this logic actually work? I think it should restart the process
         recreate_process(process_fname, args);
         int child_exit_status = 0;
         if (::wait(&child_exit_status)) {
             int signal = WTERMSIG(child_exit_status);


             // this is pretty ugly... must be better way of looping
             switch (signal)
             {
                case SIGILL:
                case SIGABRT:
                case SIGFPE:
                case SIGSEGV:
                case SIGPIPE:
                    break;
                default:;
             }
         }
    } while (/* need to count restards among other things */);

    return 0;
}
/* D:
 * - main logic for user input and main loop logic for restarting process is there
 * - basic support for using user given arguements is included
 */

/* TBD:
 * - need to write a parser/find a good one
 * - need a working/running version of the looper
 * - need to add user input for delay
 * - need to add monitor for process settings, currently logic only
 *   supports restarting a crashed process..
 *      - need to clean up code to separate this section "deployment manager"
 *      and the actual monitoring section.
 * - needs log to display fault information.
 */
