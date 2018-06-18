// @author  Gal Aharoni
// @date    04/2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>


int OUTPUT_SIZE = 256;
char SYM_COUNT[] = "./sym_count";

int i = 0;
int n_pids = 0;
int *pids = NULL;
int *pipes = NULL;

void free_resources() {
    if (pids) free(pids);
    if (pipes) free(pipes);
    for (i = 0; i < n_pids; i++) {
        close(pipes[i]);
    }
}

void sigpipe_handler(int signum) {
    printf("SIGPIPE for Manager process %d. Leaving.\n", getpid());
    for (i = 0; i < n_pids; i++) {
        kill(pids[i], SIGTERM);
    }
    free_resources();
    exit(-1);
}

int main(int argc, char *argv[])
{
    char *filename = argv[1];
    char *pattern = argv[2];
    n_pids = strlen(argv[2]);
    pids = malloc(sizeof(int) * n_pids);
    if (!pids) {
        printf("malloc failed.\n");
        free_resources();
        return -1;
    }
    pipes = malloc(sizeof(int) * n_pids);
    if (!pipes) {
        printf("malloc failed.\n");
        free_resources();
        return -1;
    }
    // register SIGPIPE handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    sigfillset(&action.sa_mask);
    action.sa_handler = sigpipe_handler;
    if (sigaction(SIGPIPE, &action, NULL) != 0) {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return errno;
    }
    int pid = 0;
    // create process & pipe for each symbol
    for (i = 0; i < n_pids; i++) {
        int p[2];
        if (pipe(p) == -1) {
			printf("pipe error: %s\n", strerror(errno));
            free_resources();
            return -1;
		}
        pid = fork();
        if (pid < 0) {
            printf("fork error: %s\n", strerror(errno));
            free_resources();
            return -1;
        } else if (pid == 0) {
            close(p[0]);
            char symbol[2] = { pattern[i], '\0' };
            char pipe_fd[32];
            sprintf(pipe_fd, "%d", p[1]); // yields writing pipe to child
			char *args[] = { SYM_COUNT, filename, symbol, pipe_fd, NULL };
			if (execvp(SYM_COUNT, args) == -1) {
				printf("execv error: %s\n", strerror(errno));
                free_resources();
                return -1;
			}
        } else { // pid == 0, parent process
            pids[i] = pid;
            pipes[i] = p[0]; // only save reading pipe
			close(p[1]);
        }
    }
    sleep(1);
    // wait all processes
    int status;
    while (n_pids) {
        for (i = 0; i < n_pids; i++) {
            waitpid(pids[i], &status, WUNTRACED);
            if (WIFEXITED(status)) {
                char output[OUTPUT_SIZE];
                if (read(pipes[i], output, OUTPUT_SIZE) < 0) {
                    printf("read error: %s\n", strerror(errno));
                    free_resources();
                    return -1;
                } else {
                    printf("%s", output);
                }
                close(pipes[i]);
                // "remove" the closed pipe
                pids[i] = pids[n_pids - 1];
                pipes[i] = pipes[n_pids - 1];
                n_pids--;
            }
        }
    }
    free_resources();
    return 0;
}
