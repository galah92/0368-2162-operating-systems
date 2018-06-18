// @author  Gal Aharoni
// @date    05/2018

#ifndef MSG_SLOT_H
#define MSG_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 200

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "msgslot"
#define DEVICE_FILE_NAME "slot"
#define MSG_SIZE (128)
#define NUM_MSGS (4)

#endif
