#include "jobs.h"
#include <stdio.h>
#include <string.h>

#define MAX_JOBS 100

static Job jobList[MAX_JOBS];
static int jobCount = 0;
void mark_job_stopped(pid_t pid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].pid == pid) {
            jobList[i].running = 0;
            return;
        }
    }
}

void add_job(pid_t pid, pid_t pgid, const char *cmd) {
    if (jobCount >= MAX_JOBS) return;

    jobList[jobCount].id = jobCount + 1;
    jobList[jobCount].pid = pid;
    jobList[jobCount].pgid = pgid;
    jobList[jobCount].running = 1;

    strncpy(jobList[jobCount].command, cmd, 255);
    jobList[jobCount].command[255] = '\0';

    printf("[+] Job %d started (PID %d, PGID %d): %s\n",
           jobList[jobCount].id,
           pid, pgid,
           jobList[jobCount].command);

    jobCount++;
}

void list_jobs() {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].running) {
            printf("[%d] PID: %d  PGID: %d  RUNNING  %s\n",
                jobList[i].id,
                jobList[i].pid,
                jobList[i].pgid,
                jobList[i].command);
        }
    }
}

Job* get_job(int id) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].id == id)
            return &jobList[i];
    }
    return NULL;
}

void remove_job(pid_t pid) {
    for (int i = 0; i < jobCount; i++) {
        if (jobList[i].pid == pid) {
            jobList[i].running = 0;
            return;
        }
    }
}

