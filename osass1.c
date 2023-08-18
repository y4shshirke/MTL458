#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLEN 2049
#define MAXWORDS 100
#define MAXBUFFER 1000

char *historyBuffer[MAXBUFFER];
char lastDirectory[MAXLEN];
int bufferPointer = 0;

void getDirectory(char *dir) {
    for (int i = 0; i < MAXLEN; i++) {
        dir[i] = '\0';
    }
    getcwd(dir, MAXLEN);
}

void prse(char *inputCmd, char **inputParsed) {
    for (int i = 0; i < MAXWORDS; i++) {
        inputParsed[i] = strsep(&inputCmd, " ");
        if (inputParsed[i] == NULL) {
            break;
        }
        if (strlen(inputParsed[i]) == 0) {
            i--;
        }
    }
    return;
}

int prsePipe(char *inputCmd, char **inputParsed) {
    for (int i = 0; i < 2; i++) {
        inputParsed[i] = strsep(&inputCmd, "|");

        if (inputParsed[i] == NULL) {
            break;
        }
        if (strlen(inputParsed[i]) == 0) {
            i--;
        }
    }

    if (inputParsed[1] == NULL) {
        return 0;
    } else {
        return 1;
    }
}

void changeDirectory(char **inputParse) {
    if (inputParse[1][0] == '~') {
        getDirectory(lastDirectory);
        chdir(getenv("HOME"));
    } else if (!strcmp(inputParse[1], "-")) {
        char temp[MAXLEN];
        getDirectory(temp);

        if (chdir(lastDirectory) < 0) {
            printf("There is no last used Directory or The File Path Does not exist for 1st Command\n");
        }

        for (int i = 0; i < MAXLEN; i++) {
            lastDirectory[i] = temp[i];
        }
    } else {
        getDirectory(lastDirectory);
        if (chdir(inputParse[1]) < 0) {
            printf("The File Path Does not exist for 1st Command\n");
        }
    }
    return;
}

void history(char **inputParse) {
    if (inputParse[1] == NULL) {
        for (int i = 0; i <= bufferPointer - 2; i++) {
            printf("%s\n", historyBuffer[i]);
        }
    } else {
        if (strlen(inputParse[1]) == 1 && inputParse[1][0] == '0') {
            exit(0);
        }
        int last = atoi(inputParse[1]);
        if (last <= 0) {
            printf("%s\n", "Invalid History Command");
            exit(0);
        }

        int start = bufferPointer - last - 1;
        if (start < 0) {
            start = 0;
        }
        for (int i = start; i <= bufferPointer - 2; i++) {
            printf("%s\n", historyBuffer[i]);
        }
    }
    return;
}

void execPipeInput(char *inp, char **inputParse1, char **inputParse2, char **chkPipe) {
    prse(chkPipe[0], inputParse1);
    prse(chkPipe[1], inputParse2);

    int fileDescriptors[2];
    pid_t child1, child2;
    int status1;
    int invFlg = 0;

    if (pipe(fileDescriptors) < 0) {
        printf("Pipe could not be initialized\n");
        return;
    }

    if (!strcmp(inputParse1[0], "exit")) {
        exit(0);
    } else if (!strcmp(inputParse1[0], "cd")) {
        inputParse1[1] = &chkPipe[0][3];

        if (inputParse1[1][0] == '~') {
            getDirectory(lastDirectory);
            chdir(getenv("HOME"));
        } else if (!strcmp(inputParse1[1], "-")) {
            char temp[MAXLEN];
            getDirectory(temp);

            if (chdir(lastDirectory) < 0) {
                printf("There is no last used Directory or The File Path Does not exist for 1st Command\n");
                invFlg = 1;
            }

            for (int i = 0; i < MAXLEN; i++) {
                lastDirectory[i] = temp[i];
            }
        } else {
            getDirectory(lastDirectory);
            if (chdir(inputParse1[1]) < 0) {
                printf("The File Path Does not exist for 1st Command\n");
                invFlg = 1;
            }
        }
    } else {
        child1 = fork();

        if (child1 < 0) {
            printf("Could not fork\n");
            exit(0);
        } else if (child1 == 0) {
            close(fileDescriptors[0]);
            dup2(fileDescriptors[1], STDOUT_FILENO);
            close(fileDescriptors[1]);

            if (!strcmp(inputParse1[0], "history")) {
                history(inputParse1);
                exit(0);
            } else if (execvp(inputParse1[0], inputParse1) < 0) {
                printf("Execution failed for 1st command\n");
                exit(1);
            }
        }
    }

    if (invFlg) {
        return;
    }

    wait(&status1);

    if (WIFEXITED(status1) && WEXITSTATUS(status1) == 0) {
        if (!strcmp(inputParse2[0], "exit")) {
            exit(0);
        } else if (!strcmp(inputParse2[0], "cd")) {
            inputParse2[1] = &chkPipe[1][3];
            changeDirectory(inputParse2);
            return;
        } else if (!strcmp(inputParse2[0], "history")) {
            history(inputParse2);
            return;
        }

        child2 = fork();

        if (child2 < 0) {
            printf("Could not fork\n");
            return;
        } else if (child2 == 0) {
            close(fileDescriptors[1]);
            dup2(fileDescriptors[0], STDIN_FILENO);
            close(fileDescriptors[0]);

            if (execvp(inputParse2[0], inputParse2) < 0) {
                printf("Execution failed for 2nd Command\n");
                exit(0);
            }
        }
    } else {
        printf("Execution failed for 1st command\n");
        return;
    }

    close(fileDescriptors[0]);
    close(fileDescriptors[1]);

    wait(NULL);

    return;
}

void execInput(char *inputCommand, char **inputParse) {
    prse(inputCommand, inputParse);
    if (!strcmp(inputParse[0], "exit")) {
        exit(0);
        return;
    } else if (!strcmp(inputParse[0], "cd")) {
        inputParse[1] = &inputCommand[3];
        changeDirectory(inputParse);
        return;
    }

    pid_t child1 = fork();

    if (child1 < 0) {
        printf("Fork Failed");
    } else if (child1 == 0) {
        if (!strcmp(inputParse[0], "history")) {
            history(inputParse);
        } else {
            if (execvp(inputParse[0], inputParse) < 0) {
                printf("Invalid Command\n");
            }
        }
        exit(0);
    } else {
        wait(NULL);
        return;
    }
}

int main() {
    char inputCommand[MAXLEN], *inputParse1[MAXWORDS];
    char *inputParse2[MAXWORDS], *chkPipe[2];
    int pipeFlag = 0;
    getDirectory(lastDirectory);

    while (1) {
        printf("MTL458 >");

        fgets(inputCommand, MAXLEN, stdin);

        if (inputCommand[strlen(inputCommand) - 1] == '\n')
            inputCommand[strlen(inputCommand) - 1] = '\0';

        historyBuffer[bufferPointer] = strdup(inputCommand);
        bufferPointer++;

        pipeFlag = prsePipe(inputCommand, chkPipe);

        if (!pipeFlag) {
            execInput(inputCommand, inputParse1);
        } else {
            execPipeInput(inputCommand, inputParse1, inputParse2, chkPipe);
        }
    }

    return 0;
}
