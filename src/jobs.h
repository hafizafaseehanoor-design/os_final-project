#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct Job {
    int id;
    pid_t pid;     // one process inside job
    pid_t pgid;    // process group ID (critical)
    int running;
    char command[256];
} Job;
void mark_job_stopped(pid_t pid);
void add_job(pid_t pid, pid_t pgid, const char *cmd);
void list_jobs();
Job* get_job(int id);
void remove_job(pid_t pid);

#endif

