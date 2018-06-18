// @author  Gal Aharoni
// @date    05/2018

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include "message_slot.h"


int main(int argc, const char *argv[])
{
    const char *path = argv[1];
    const int channel = atoi(argv[2]);
    const char *message = argv[3];
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        printf("Failed open device file: %s\n", path);
        return -1;
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) != 0)
    {
        printf("Failed set channed ID\n");
        return -1;
    }
    if (write(fd, message, strlen(message)) != 0) {
        printf("Failed write message\n");
        return -1;
    }
    close(fd);
    printf("%u bytes written to %s\n", strlen(message), path);
    return 0;
}
