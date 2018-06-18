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
    char message[MSG_SIZE];
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
    if (read(fd, message, MSG_SIZE) != 0) {
        printf("Failed read message\n");
        return -1;
    }
    close(fd);
    printf("%u bytes read from %s\n%s\n", strlen(message), path, message);
    return 0;
}
