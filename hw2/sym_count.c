// @author  Gal Aharoni
// @date    04/2018

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>


int OUTPUT_SIZE = 256;

char symbol = '\0';
int counter = 0;
int fd = 0;
char *arr = NULL;
int filesize = 0;

void sigpipe_handler(int signum) {
    printf("SIGPIPE for process %d. Symbol %c. Counter %d. Leaving.\n",
        getpid(),
        symbol,
        counter);
    munmap(arr, filesize);
    close(fd);
    exit(0);
}

int main(int argc, const char *argv[]) {
    int i;
    int result;
    const char *filename = argv[1];
    symbol = argv[2][0];
    int pipe_fd = atoi(argv[3]);
    // register SIGPIPE handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    sigfillset(&action.sa_mask);
    action.sa_handler = sigpipe_handler;
    if (sigaction(SIGPIPE, &action, NULL) != 0) {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return errno;
    }
    // getting file size
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("stat error: %s\n", strerror(errno));
        return -1;
    }
    filesize = st.st_size;
    // mmapping the file
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Error opening file: %s\n", strerror(errno));
        return -1;
    }
    arr = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (arr == MAP_FAILED) {
        printf("Error mmapping the file: %s\n", strerror(errno));
        return -1;
    }
    // counting symbols
    for (i = 0; i < filesize; ++i) {
        counter += arr[i] == argv[2][0];
    }
    // writing to pipe
    char output[OUTPUT_SIZE];
    sprintf(output, "Process %d finishes. Symbol %c. Instances %d.\n",
            getpid(),
            symbol,
            counter);
    if (write(pipe_fd, output, strlen(output)) != strlen(output)) {
        printf("Could not write to pipe. %s\n", strerror(errno));
    }
    // releasing resources and exiting
    if (munmap(arr, filesize) == -1) {
        printf("Error un-mmapping the file: %s\n", strerror(errno));
        return -1;
    }
    close(fd);
    return 0;
}
