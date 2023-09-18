//Brandon Nguyen
//Krystal Phan
//Unix Shell Project
//Due Date: 9/17/2023

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int findPipe(char** commandArgs);

void parseInput(char* input, char** commandArgs, char** redirection);

void redirectToOutput(char* filename);

void redirectToInput(char* filename);

void executeSingleCommand(char** commandArgs);

void executePipedCommands(char** commandArgs);

int findPipe(char** commandArgs) {
    int index = 0;

    while (commandArgs[index] != NULL) {
        if (!strcmp(commandArgs[index], "|")) {
            return index;
        }
        index++;
    }

    return -1;
}

void parseInput(char* input, char** commandArgs, char** redirection) {
    const char delimiters[3] = {' ', '\t', '\n'};
    char* token;
    int numArgs = 0;
    token = strtok(input, delimiters);

    while (token != NULL) {
        if (!strcmp(token, ">")) {
            token = strtok(NULL, delimiters);
            redirection[0] = "o";
            redirection[1] = token;
            break;
        }
        else if (!strcmp(token, "<")) {
            token = strtok(NULL, delimiters);
            redirection[0] = "i";
            redirection[1] = token;
            break;
        }
        else if (!strcmp(token, "|")) {
            redirection[0] = "p";
            break;
        }
        commandArgs[numArgs++] = token;
        token = strtok(NULL, delimiters);
    }
    commandArgs[numArgs] = NULL;
}

void redirectToOutput(char* filename) {
    printf("Output saved to ./%s\n", filename);
    int fileDescriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fileDescriptor < 0) {
        perror("open");
        exit(1);
    }

    dup2(fileDescriptor, STDOUT_FILENO);
    close(fileDescriptor);
}

void redirectToInput(char* filename) {
    printf("Reading from file: ./%s\n", filename);
    int fileDescriptor = open(filename, O_RDONLY);

    if (fileDescriptor < 0) {
        perror("open");
        exit(1);
    }

    dup2(fileDescriptor, STDIN_FILENO);
    close(fileDescriptor);
}

void executeSingleCommand(char** commandArgs) {
    execvp(commandArgs[0], commandArgs);
    perror("execvp");
    exit(1);
}

void executePipedCommands(char** commandArgs) {
    char* leftHandSide[BUFFER_SIZE];
    char* rightHandSide[BUFFER_SIZE];

    int pipeOffset = findPipe(commandArgs);
    commandArgs[pipeOffset] = NULL;
    int pipeFileDescriptors[2];

    if (pipe(pipeFileDescriptors) < 0) {
        perror("pipe");
        exit(1);
    }

    pid_t childPid = fork();

    if (childPid < 0) {
        perror("fork");
        exit(1);
    }

    if (childPid == 0) {
        close(pipeFileDescriptors[0]);

        dup2(pipeFileDescriptors[1], STDOUT_FILENO);
        close(pipeFileDescriptors[1]);

        executeSingleCommand(commandArgs);
    }
    else {
        close(pipeFileDescriptors[1]);

        int status;
        waitpid(childPid, &status, 0);

        if (status != 0) {
            fprintf(stderr, "Child process failed\n");
        }

        int bytesRead;
        char buffer[BUFFER_SIZE];

        while ((bytesRead = read(pipeFileDescriptors[0], buffer, BUFFER_SIZE)) > 0) {
            write(STDOUT_FILENO, buffer, bytesRead);
        }

        close(pipeFileDescriptors[0]);
    }
    exit(0);
}

int main(int argc, const char* argv[]) {
    char input[BUFFER_SIZE];
    char previousCommand[BUFFER_SIZE];

    memset(input, 0, BUFFER_SIZE * sizeof(char));
    memset(previousCommand, 0, BUFFER_SIZE * sizeof(char));

    while (true) {
        printf("osh> ");
        fflush(stdout);
        fgets(input, BUFFER_SIZE, stdin);

        input[strlen(input) - 1] = '\0';

        if (!strcmp(input, "exit")) {
            return 0;
        }

        if (!strcmp(input, "!!")) {
            if (previousCommand[0] == '\0') {
                printf("No recently used commands\n");
                continue;
            }
            strcpy(input, previousCommand);
        }
        else {
            strcpy(previousCommand, input);
        }

        bool isWaiting = true;
        char* waitOffset = strchr(input, '&');

        if (waitOffset != NULL) {
            *waitOffset = ' ';
            isWaiting = false;
        }

        pid_t processId = fork();

        if (processId < 0) {
            fprintf(stderr, "Failed to fork\n");
            return -1;
        }

        if (processId != 0) {
            if (isWaiting) {
                wait(NULL);
            }
        }
        else {
            char* commandArgs[BUFFER_SIZE];
            char* redirection[2] = { "", "" };

            parseInput(input, commandArgs, redirection);

            if (!strcmp(redirection[0], "o")) {
                redirectToOutput(redirection[1]);
            }
            else if (!strcmp(redirection[0], "i")) {
                redirectToInput(redirection[1]);
            }
            else if (!strcmp(redirection[0], "p")) {
                executePipedCommands(commandArgs);
            }
            else {
                executeSingleCommand(commandArgs);
            }
        }
    }
    return 0;
}

//osc@ubuntu:~/final-src-osc10e/ch3$ gcc unixshell.c -o unixshell
// osc@ubuntu:~/final-src-osc10e/ch3$ ./unixshell 
// osh> pwd
// /home/osc/final-src-osc10e/ch3
// osh> ls
// DateClient.java  fig3-30.c  fig3-32.c  fig3-34.c  multi-fork    newproc-posix.c  pid.c                 shm-posix-producer.c  unix_pipe.c  unixshell.c         win32-pipe-parent.c
// DateServer.java  fig3-31.c  fig3-33.c  fig3-35.c  multi-fork.c  newproc-win32.c  shm-posix-consumer.c  simple-shell.c        unixshell    win32-pipe-child.c
// osh> cal 2023
//                             2023
//       January               February               March          
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//  1  2  3  4  5  6  7            1  2  3  4            1  2  3  4  
//  8  9 10 11 12 13 14   5  6  7  8  9 10 11   5  6  7  8  9 10 11  
// 15 16 17 18 19 20 21  12 13 14 15 16 17 18  12 13 14 15 16 17 18  
// 22 23 24 25 26 27 28  19 20 21 22 23 24 25  19 20 21 22 23 24 25  
// 29 30 31              26 27 28              26 27 28 29 30 31     
                                                                  

//        April                  May                   June          
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//                    1      1  2  3  4  5  6               1  2  3  
//  2  3  4  5  6  7  8   7  8  9 10 11 12 13   4  5  6  7  8  9 10  
//  9 10 11 12 13 14 15  14 15 16 17 18 19 20  11 12 13 14 15 16 17  
// 16 17 18 19 20 21 22  21 22 23 24 25 26 27  18 19 20 21 22 23 24  
// 23 24 25 26 27 28 29  28 29 30 31           25 26 27 28 29 30     
// 30                                                                

//         July                 August              September        
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//                    1         1  2  3  4  5                  1  2  
//  2  3  4  5  6  7  8   6  7  8  9 10 11 12   3  4  5  6  7  8  9  
//  9 10 11 12 13 14 15  13 14 15 16 17 18 19  10 11 12 13 14 15 16  
// 16 17 18 19 20 21 22  20 21 22 23 24 25 26  17 18 19 20 21 22 23  
// 23 24 25 26 27 28 29  27 28 29 30 31        24 25 26 27 28 29 30  
// 30 31                                                             

//       October               November              December        
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//  1  2  3  4  5  6  7            1  2  3  4                  1  2  
//  8  9 10 11 12 13 14   5  6  7  8  9 10 11   3  4  5  6  7  8  9  
// 15 16 17 18 19 20 21  12 13 14 15 16 17 18  10 11 12 13 14 15 16  
// 22 23 24 25 26 27 28  19 20 21 22 23 24 25  17 18 19 20 21 22 23  
// 29 30 31              26 27 28 29 30        24 25 26 27 28 29 30  
//                                             31                    
// osh> !!
//                             2023
//       January               February               March          
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//  1  2  3  4  5  6  7            1  2  3  4            1  2  3  4  
//  8  9 10 11 12 13 14   5  6  7  8  9 10 11   5  6  7  8  9 10 11  
// 15 16 17 18 19 20 21  12 13 14 15 16 17 18  12 13 14 15 16 17 18  
// 22 23 24 25 26 27 28  19 20 21 22 23 24 25  19 20 21 22 23 24 25  
// 29 30 31              26 27 28              26 27 28 29 30 31     
                                                                  

//        April                  May                   June          
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//                    1      1  2  3  4  5  6               1  2  3  
//  2  3  4  5  6  7  8   7  8  9 10 11 12 13   4  5  6  7  8  9 10  
//  9 10 11 12 13 14 15  14 15 16 17 18 19 20  11 12 13 14 15 16 17  
// 16 17 18 19 20 21 22  21 22 23 24 25 26 27  18 19 20 21 22 23 24  
// 23 24 25 26 27 28 29  28 29 30 31           25 26 27 28 29 30     
// 30                                                                

//         July                 August              September        
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//                    1         1  2  3  4  5                  1  2  
//  2  3  4  5  6  7  8   6  7  8  9 10 11 12   3  4  5  6  7  8  9  
//  9 10 11 12 13 14 15  13 14 15 16 17 18 19  10 11 12 13 14 15 16  
// 16 17 18 19 20 21 22  20 21 22 23 24 25 26  17 18 19 20 21 22 23  
// 23 24 25 26 27 28 29  27 28 29 30 31        24 25 26 27 28 29 30  
// 30 31                                                             

//       October               November              December        
// Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  
//  1  2  3  4  5  6  7            1  2  3  4                  1  2  
//  8  9 10 11 12 13 14   5  6  7  8  9 10 11   3  4  5  6  7  8  9  
// 15 16 17 18 19 20 21  12 13 14 15 16 17 18  10 11 12 13 14 15 16  
// 22 23 24 25 26 27 28  19 20 21 22 23 24 25  17 18 19 20 21 22 23  
// 29 30 31              26 27 28 29 30        24 25 26 27 28 29 30  
//                                             31                    
// osh> exit
// osc@ubuntu:~/final-src-osc10e/ch3$ 

// osh> ls -l | out.txt
// total 120
// -rw-rw-r-- 1 osc osc   710 Jan  3  2018 DateClient.java
// -rw-rw-r-- 1 osc osc   810 Jun 18  2018 DateServer.java
// -rw-rw-r-- 1 osc osc   361 Jun 18  2018 fig3-30.c
// -rw-rw-r-- 1 osc osc   121 Jan  3  2018 fig3-31.c
// -rw-rw-r-- 1 osc osc   136 Jan  3  2018 fig3-32.c
// -rw-rw-r-- 1 osc osc   500 Jun 18  2018 fig3-33.c
// -rw-rw-r-- 1 osc osc   680 Jun 18  2018 fig3-34.c
// -rw-rw-r-- 1 osc osc   534 Jun 18  2018 fig3-35.c
// -rw-rw-r-- 1 osc osc     8 Sep 17 16:39 in.txt
// -rwxrwxr-x 1 osc osc  8712 Jan 30  2018 multi-fork
// -rw-rw-r-- 1 osc osc   257 Jan 30  2018 multi-fork.c
// -rw-rw-r-- 1 osc osc   780 Jan 28  2018 newproc-posix.c
// -rw-rw-r-- 1 osc osc  1413 Jan  3  2018 newproc-win32.c
// -rw-rw-r-- 1 osc osc     8 Sep 17 16:40 out.txt
// -rw-r--r-- 1 osc osc  2976 Jun 18  2018 pid.c
// -rw-rw-r-- 1 osc osc  1115 Jun 18  2018 shm-posix-consumer.c
// -rw-rw-r-- 1 osc osc  1434 Jun 18  2018 shm-posix-producer.c
// -rw-rw-r-- 1 osc osc   707 Jun 18  2018 simple-shell.c
// -rw-rw-r-- 1 osc osc  1219 Jan  3  2018 unix_pipe.c
// -rwxrwxr-x 1 osc osc 14240 Sep 17 16:06 unixshell
// -rw-rw-r-- 1 osc osc 10393 Sep 17 16:20 unixshell.c
// -rw-rw-r-- 1 osc osc   755 Jan  3  2018 win32-pipe-child.c
// -rw-rw-r-- 1 osc osc  2236 Jan  3  2018 win32-pipe-parent.c
// osh> ls -l > out.txt
// Output saved to ./out.txt
