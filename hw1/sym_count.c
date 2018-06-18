// @author      Gal Aharoni
// @date        04/2018
// @compile     gcc sym_count.c -o sym_count
// @usage       ./sym_count <file-path> <symbol>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

char symbol;
int counter = 0;
int fd;

void signal_handler(int signum) {
    switch (signum) {
        case SIGTERM:
            printf("Process %d finishes. Symbol %c. Instances %d.\n",
                   getpid(),
                   symbol,
                   counter);
            close(fd);
            exit(0);
        case SIGCONT:
            printf("Process %d continues\n", getpid());
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./sym_count <file-path> <symbol>\n");
        return -1;
    }
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    sigfillset(&action.sa_mask);
    action.sa_handler = signal_handler;
    if ((sigaction(SIGTERM, &action, NULL) != 0) ||
        (sigaction(SIGCONT, &action, NULL) != 0)) {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return errno;
    }
    symbol = argv[2][0];
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("Error opening file: %s\n", strerror(errno));
        return errno;
    }
    char buffer[BUFFER_SIZE];
    ssize_t numOfBytes;
    while (numOfBytes = read(fd, buffer, BUFFER_SIZE)) {
        if (numOfBytes < 0) {
            printf("Error reading from file: %s\n", strerror(errno));
            close(fd);
            return errno;
        }
        int i;
        for (i = 0; i < numOfBytes; i++) {
            if (buffer[i] == symbol) {
                counter++;
                printf("Process %d, symbol %c, going to sleep\n", getpid(), symbol);
                raise(SIGSTOP);
            }
        }
    }
    raise(SIGTERM);
    return -1; // should exit through SIGTERM
}
