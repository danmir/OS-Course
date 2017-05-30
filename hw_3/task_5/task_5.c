#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>

/* Количество попыток для запуска */
#define __MAX_RETRY_ATTEMPTS__ 50

/* Время ожидания после неудачных попыток */
#define __WAIT_TIME__ 3600

#define __MAX_PROCS__ 100
#define __MAX_COMMAND_LEN__ 100
#define __MAX_COMMAND_ARGS_COUNT__ 100
#define __MAX_FILE_NAME_LEN__ 100

struct watcher_proc {
    pid_t pid;
    char command[__MAX_COMMAND_LEN__];
    char* command_args[__MAX_COMMAND_ARGS_COUNT__];
    int watch_policy; // respawn -> 1; sleep -> 0;
    int tries_count;
};

int watcher_procs_count = 0;
int proc_counter = 0; // Количество запущенных процессов
struct watcher_proc watcher_procs[__MAX_PROCS__];
char* conf_file_path;


char* get_file_name(int proc_num) {
    char *file_name = (char*)malloc((size_t) __MAX_FILE_NAME_LEN__);
    sprintf(file_name, "/tmp/watcher.%d.pid", watcher_procs[proc_num].pid);
    return file_name;
}

void create_file(int proc_num) {
    char *file_name = (char*)malloc((size_t) __MAX_FILE_NAME_LEN__);
    strcpy(file_name, get_file_name(proc_num));

    FILE *fp = fopen(file_name, "wa");

    if (fp == NULL) {
        syslog(LOG_ERR, "Can't open file: %s\n", file_name);
        exit(1);
    }

    fprintf(fp, "%d", watcher_procs[proc_num].pid);
    fclose(fp);
    return;
}

void remove_file(int proc_num) {
    char *file_name = (char*)malloc((size_t) __MAX_FILE_NAME_LEN__);
    strcpy(file_name, get_file_name(proc_num));

    if (remove(file_name) < 0) {
        syslog(LOG_ERR, "Can't delete %s\n", file_name);
        exit(1);
    }
    return;
}

void set_conf_file_path(int argc, char *argv[]) {
    if (argc != 2) {
        syslog(LOG_ERR, "%s\n", "Can't get conf file parameter");
        exit(1);
    }
    conf_file_path = argv[1];
    return;
}

void parse_conf() {
    FILE *fp = fopen(conf_file_path, "r");
    if (fp == NULL) {
        syslog(LOG_ERR, "Can't open conf file for reading\n");
        exit(1);
    };

    char* line = NULL;
    size_t len = 0;
    int iter = 0;

    while ((getline(&line, &len, fp)) != -1) {
        char sep[] = " ";
        char* token = strtok(line, sep);

        bool was_compared = false;
        if (strncmp(token, "respawn", 7) == 0) {
            watcher_procs[iter].watch_policy = 1;
            was_compared = true;
        }
        if (strncmp(token, "wait", 4) == 0) {
            watcher_procs[iter].watch_policy = 0;
            was_compared = true;
        }
        if (!was_compared) {
            syslog(LOG_ERR, "Invalid conf file\n");
            exit(1);
        }

        watcher_procs[iter].tries_count = 0;
        watcher_procs[iter].pid = 0;

        // Разберем строку с коммандой на саму комманду и аргументы
        char* args_token = strtok(NULL, sep);
        strcpy(watcher_procs[iter].command, args_token);
        for (int i = 0; i < __MAX_COMMAND_ARGS_COUNT__; ++i) {
            if (args_token != NULL) {

                // Убрем \n в конце последнего аргумента
                for (int j = 0; j < strlen(args_token); ++j) {
                    if (args_token[j] == '\n') {
                        args_token[j] = '\0';
                        break;
                    }
                }

                watcher_procs[iter].command_args[i] = (char *)malloc(strlen(args_token));
                strcpy(watcher_procs[iter].command_args[i], args_token);
                args_token = strtok(NULL, sep);
            } else {
                watcher_procs[iter].command_args[i] = NULL;
                break;
            }
        }
        iter++;
    }

    watcher_procs_count = iter;
    fclose(fp);
    return;
}

void spawn(int proc_num, bool do_need_sleep) {
    int cpid = fork();
    switch (cpid) {
        case -1:
            syslog(LOG_ERR, "%s\n", "Can't fork for subproc");
            exit(1);
        case 0:
            if (do_need_sleep) {
                syslog(LOG_ERR, "%s %s\n", "Can't start", watcher_procs[proc_num].command);
                sleep(__WAIT_TIME__);
                watcher_procs[proc_num].tries_count = 0;
            }

            if (execvp(watcher_procs[proc_num].command, watcher_procs[proc_num].command_args) < 0) {
                syslog(LOG_ERR, "Can't exec program %s with args: %s\n", watcher_procs[proc_num].command,
                       (char *) watcher_procs[proc_num].command_args);
                exit(1);
            }

            syslog(LOG_NOTICE, "%d spawned, pid = %d\n", proc_num, cpid);
            exit(0);
        default:
            watcher_procs[proc_num].pid = cpid;
            create_file(proc_num);
            proc_counter++;
    }
}

void run_tasks() {
    watcher_procs_count = 0;
    proc_counter = 0;

    parse_conf();

    for (int i = 0; i < watcher_procs_count; ++i) {
        spawn(i, false);
    }

    while (proc_counter) {
        int status;
        pid_t cpid = wait(&status);
        //syslog(LOG_ERR, "Status: %d %d\n", status, errno);

        for (int i = 0; i < watcher_procs_count; ++i) {
            if (watcher_procs[i].pid == cpid) {
                syslog(LOG_NOTICE, "Child %d with pid %d was finished\n", i, cpid);
                remove_file(i);
                proc_counter--;

                if (watcher_procs[i].watch_policy == 1) {
                    if (status != 0) {
                        watcher_procs[i].tries_count++;
                    }

                    if (watcher_procs[i].tries_count >= __MAX_RETRY_ATTEMPTS__) {
                        spawn(i, true);
                    } else {
                        spawn(i, false);
                    }
                }
            }
        }
    }

//    printf("All tasks finished \n");
    syslog(LOG_INFO, "All tasks finished \n");
}

void sig_handler(int sig) {
//    printf("HUB signal\n");
    for (int i = 0; i < watcher_procs_count; i++) {
        if (watcher_procs[i].pid > 0) {
            kill(watcher_procs[i].pid, SIGKILL);
        }
    }
    run_tasks();
}


int main(int argc, char *argv[]) {
    signal(SIGHUP, (void (*)(int)) sig_handler);
    set_conf_file_path(argc, argv);

    // Делаем демона
    switch (fork()) {
        case -1:
            syslog(LOG_ERR, "%s\n", "Can't start daemon");
            exit(1);
        case 0:
            setsid();
            chdir("/");
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            run_tasks();
            exit(0);
        default:
            return 0;
    }
}