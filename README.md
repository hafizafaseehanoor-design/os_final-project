Custom Unix Shell — Operating Systems Project

Author: Hafiza Faseeha Noor
Course: Operating Systems
Project: Mini Unix Shell in C

Overview

This project implements a custom Unix-like shell written in C.
It supports real process execution, pipelines, redirection, job control, and background tasks  similar to how Bash works.

The shell was built from scratch using fork(), execvp(), waitpid(), signals, and process groups.

Features:
 1. Run External Commands
ls
pwd
cat file.txt
echo "hello world"

 2.Built-In Commands
 Command    Description              

 `cd`       Change directory         
 `clear`    Clear terminal screen    
 `help`     Show help                
 `exit`     Exit shell               
 `jobs`     List running jobs        
 `fg <id>`  Bring job to foreground  
 `bg <id>`  Resume job in background 
3. Background Execution (&)
sleep 5 &
ping google.com &
4. Job Control
myshell> sleep 5 &
[+] Job 1 started (PID 18855, PGID 18855): sleep

myshell> jobs
[1] PID: 18855  PGID: 18855  RUNNING  sleep

5. Foreground / Background Process Groups

Each job runs in its own process group, enabling proper signal handling.

Foreground job receives input and signals (Ctrl+C, Ctrl+Z)

Shell ignores Ctrl+C/Ctrl+Z
6. Redirection
echo hello > out.txt
wc -l < out.txt
echo world >> out.txt
7. Pipelines ( | )

Supports multi-stage pipelines:

ls | wc -l
cat file.txt | grep hello | sort
echo hey | tr a-z A-Z



os_final-project/
 ├── src/
 │   ├── main.c      (shell main logic)
 │   ├── parser.c    (tokenizer + pipeline parser)
 │   ├── parser.h
 │   ├── jobs.c      (job table, bg/fg, PGID management)
 │   ├── jobs.h
 ├── Makefile
 └── README.md

Compilation & Running

Open terminal:

cd os_final-project
make
./myshell


To exit:

exit
