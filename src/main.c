#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

#include "parser.h"
#include "jobs.h"

#define MAX_INPUT 1024

// ------------------ REDIRECTION ------------------
void handle_redirection(char **tokens, int count) {
    for (int i = 0; i < count; i++) {
        if (!tokens[i]) continue;

        if (strcmp(tokens[i], "<") == 0) {
            int fd = open(tokens[i+1], O_RDONLY);
            if (fd < 0) { perror("open <"); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
            tokens[i] = NULL;
        }
        else if (strcmp(tokens[i], ">") == 0) {
            int fd = open(tokens[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if (fd < 0) { perror("open >"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            tokens[i] = NULL;
        }
        else if (strcmp(tokens[i], ">>") == 0) {
            int fd = open(tokens[i+1], O_WRONLY|O_CREAT|O_APPEND, 0644);
            if (fd < 0) { perror("open >>"); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            tokens[i] = NULL;
        }
    }
}

int main() {
    char input[MAX_INPUT];

    // shell becomes process group leader
    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    // ignore signals INSIDE the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);   // <-- REQUIRED FIX
    signal(SIGTTIN, SIG_IGN);   // <-- REQUIRED FIX

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0)
            continue;

        // ------------------ PIPELINES ------------------
        char segments[10][1024];
        int segCount = split_pipeline(input, segments);

        if (segCount >= 2) {
            int pipefd[segCount - 1][2];
            pid_t pids[segCount];

            for (int i = 0; i < segCount - 1; i++)
                pipe(pipefd[i]);

            for (int i = 0; i < segCount; i++) {
                int tokCount = 0;
                char **cmdTokens = tokenize(segments[i], &tokCount);

                pids[i] = fork();
                if (pids[i] == 0) {
                    // child process
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);

                    setpgid(0, getpid());
                    tcsetpgrp(STDIN_FILENO, getpid());

                    if (i > 0)
                        dup2(pipefd[i-1][0], STDIN_FILENO);
                    if (i < segCount - 1)
                        dup2(pipefd[i][1], STDOUT_FILENO);

                    handle_redirection(cmdTokens, tokCount);

                    for (int k = 0; k < segCount - 1; k++) {
                        close(pipefd[k][0]);
                        close(pipefd[k][1]);
                    }

                    execvp(cmdTokens[0], cmdTokens);
                    perror("exec");
                    exit(1);
                }

                setpgid(pids[i], pids[0]);

                for (int j = 0; j < tokCount; j++) free(cmdTokens[j]);
                free(cmdTokens);
            }

            for (int i = 0; i < segCount - 1; i++) {
                close(pipefd[i][0]);
                close(pipefd[i][1]);
            }

            for (int i = 0; i < segCount; i++)
                waitpid(pids[i], NULL, 0);

            tcsetpgrp(STDIN_FILENO, shell_pgid);
            continue;
        }

        // ------------------ NORMAL COMMAND ------------------
        int count = 0;
        char **tokens = tokenize(input, &count);
        if (count == 0) continue;

        // exit
        if (strcmp(tokens[0], "exit") == 0) {
            for (int i = 0; i < count; i++) free(tokens[i]);
            free(tokens);
            break;
        }

        // builtins
        if (strcmp(tokens[0], "clear") == 0) {
            printf("\033[2J\033[H");
            goto cleanup;
        }

        if (strcmp(tokens[0], "help") == 0) {
            printf("Commands: help, clear, exit, cd, jobs, fg, bg\n");
            goto cleanup;
        }

        if (strcmp(tokens[0], "cd") == 0) {
            if (count == 1)
                chdir(getenv("HOME"));
            else if (chdir(tokens[1]) != 0)
                perror("cd");
            goto cleanup;
        }

        if (strcmp(tokens[0], "jobs") == 0) {
            list_jobs();
            goto cleanup;
        }

        // ------------------ fg ------------------
        if (strcmp(tokens[0], "fg") == 0) {
            if (count < 2) {
                printf("Usage: fg <id>\n");
                goto cleanup;
            }

            int id = atoi(tokens[1]);
            Job *job = get_job(id);
            if (!job) {
                printf("fg: job not found\n");
                goto cleanup;
            }

            printf("Bringing job %d to foreground...\n", id);

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            tcsetpgrp(STDIN_FILENO, job->pgid);

            int status;
            kill(-job->pgid, SIGCONT);

            waitpid(-job->pgid, &status, WUNTRACED);

            tcsetpgrp(STDIN_FILENO, shell_pgid);

            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);

            if (!WIFSTOPPED(status))
                remove_job(job->pgid);

            goto cleanup;
        }

        // ------------------ bg ------------------
        if (strcmp(tokens[0], "bg") == 0) {
            if (count < 2) {
                printf("Usage: bg <id>\n");
                goto cleanup;
            }

            int id = atoi(tokens[1]);
            Job *job = get_job(id);

            if (!job) {
                printf("bg: job not found\n");
                goto cleanup;
            }

            kill(-job->pgid, SIGCONT);
            printf("[+] Job %d resumed\n", id);
            goto cleanup;
        }

        // background?
        int background = 0;
        if (strcmp(tokens[count - 1], "&") == 0) {
            background = 1;
            free(tokens[count - 1]);
            tokens[count - 1] = NULL;
            count--;
        }

        // ------------------ EXECUTE ------------------
        pid_t pid = fork();

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            setpgid(0, getpid());

            if (!background)
                tcsetpgrp(STDIN_FILENO, getpid());

            handle_redirection(tokens, count);

            execvp(tokens[0], tokens);
            perror("exec");
            exit(1);
        }
        else {
            setpgid(pid, pid);

            if (background) {
                add_job(pid, pid, tokens[0]);
            } else {
                int status;
                tcsetpgrp(STDIN_FILENO, pid);

                waitpid(pid, &status, WUNTRACED);

                tcsetpgrp(STDIN_FILENO, shell_pgid);

                if (WIFSTOPPED(status))
                    add_job(pid, pid, tokens[0]);
            }
        }

cleanup:
        for (int i = 0; i < count; i++) free(tokens[i]);
        free(tokens);
    }

    return 0;
}
