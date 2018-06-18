// @author      Gal Aharoni
// @date        04/2018
// @compile     gcc sym_mng.c -o sym_mng
// @usage       ./sym_mng <file-path> <pattern> <bound>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

typedef struct PIDInfo {
    int pid;
    int stopCounter;
    int isRunning;
} PIDInfo;

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage: ./sym_count <file-path> <pattern> <bound>\n");
        return -1;
    }
    int terminationBound = atoi(argv[3]);
    int pid = -1;
    int numOfRunningPIDs = 0;
    int numOfTotalPIDs = strlen(argv[2]);
    if (numOfTotalPIDs == 0) {
        return -1;
    }
    PIDInfo *childPIDs = malloc(sizeof(PIDInfo) * numOfTotalPIDs);
    if (!childPIDs) {
        printf("malloc failed.\n");
        free(childPIDs);
        return -1;
    }
    char symbol[] = " "; // allocate on the stack
    char *execArgs[] = { "./sym_count", argv[1], symbol, NULL };
    int i;
    for (i = 0; i < numOfTotalPIDs; i++) {
        if ((pid = fork()) == 0) { // that's the child process
            execArgs[2][0] = argv[2][i];
            int result = execvp(execArgs[0], execArgs);
            if (result == -1) {
                printf("Error in execvp: %s\n", strerror(errno));
                for (i = 0; i < numOfRunningPIDs; i++) {
                    kill(childPIDs[i].pid, SIGTERM);
                }
                free(childPIDs);
                return errno;
            }
        } else {
            childPIDs[i].pid = pid;
            childPIDs[i].stopCounter = 0;
            childPIDs[i].isRunning = 1;
            numOfRunningPIDs++;
        }
    }
    sleep(1);
    int status;
    for (i = 0; numOfRunningPIDs > 0; i = (i + 1) % numOfTotalPIDs) {
        if (!childPIDs[i].isRunning) continue;
        waitpid(childPIDs[i].pid, &status, WUNTRACED);
        if (WIFEXITED(status)) {
            numOfRunningPIDs--;
            childPIDs[i].isRunning = 0;
        } else if (WIFSTOPPED(status)) {
            childPIDs[i].stopCounter++;
            if (childPIDs[i].stopCounter == terminationBound) {
                kill(childPIDs[i].pid, SIGTERM);
            }
            kill(childPIDs[i].pid, SIGCONT);
        }
    }
    free(childPIDs);
    return 0;
}
