// @author  Gal Aharoni
// @date    05/2018

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");


char buffer[MSG_SIZE * NUM_MSGS];

struct slot {
    unsigned long minor;
    char buffer[MSG_SIZE * NUM_MSGS];
    size_t buffer_size[NUM_MSGS];
    struct slot* next;
} sentinel = {
    .minor = NUM_MSGS,
    .next = NULL,
};

static struct slot* begin = &sentinel;

static int device_open(struct inode* inode, struct file* file)
{
    int i;
    unsigned long minor = iminor(inode);
    struct slot* slot = begin;
    file->private_data = (void*)-1;
    // get last slot in list
    while (slot->next)
    {
        if (slot->minor == minor) return 0; // already exists
        slot = slot->next;
    }
    slot->next = kmalloc(sizeof(struct slot), GFP_KERNEL);
    if (!slot->next)
    {
        printk(KERN_INFO"message_slot: kmalloc failed\n");
        return -EINVAL;
    }
    // assign default values
    slot->next->minor = minor;
    slot->next->next = NULL;
    for (i = 0; i < NUM_MSGS; i++) slot->next->buffer_size[i] = 0;
    return 0;
}

static long device_ioctl(struct file* file,
                         unsigned int ioctl_command_id,
                         unsigned long ioctl_param)
{
    if (ioctl_command_id != MSG_SLOT_CHANNEL) return -EINVAL;
    if (ioctl_param >= NUM_MSGS) return -EINVAL;
    file->private_data = (void*)ioctl_param;
    return 0;
}

static ssize_t device_read(struct file* file,
                           char __user* buffer,
                           size_t length,
                           loff_t* offset)
{
    long channel = (long)file->private_data;
    long minor = iminor(file_inode(file));
    struct slot* slot = begin;
    void *from;
    if ((long)file->private_data == -1) return -EINVAL;
    // get the relevant slot
    while (slot && slot->minor != minor) slot = slot->next;
    if (!slot->buffer_size[channel]) return -EWOULDBLOCK;
    if (slot->buffer_size[channel] > length) return -ENOSPC;
    // copy to user space
    from = slot->buffer + channel * MSG_SIZE;
    copy_to_user(buffer, from, slot->buffer_size[channel]);
    return 0;
}

static ssize_t device_write(struct file* file,
                            const char __user* buffer,
                            size_t length,
                            loff_t* offset)
{
    long channel = (long)file->private_data;
    long minor = iminor(file_inode(file));
    struct slot* slot = begin;
    if ((long)file->private_data == -1 || length > MSG_SIZE) return -EINVAL;
    // get the relevant slot and copy to kernel space
    while (slot && slot->minor != minor) slot = slot->next;
    copy_from_user(slot->buffer + channel * MSG_SIZE, buffer, length);
    slot->buffer_size[channel] = length;
    return 0;
}

static int device_release(struct inode* inode, struct file* file)
{
    return 0; // releasing everything on module_exit();
}

struct file_operations fops =
{
  .open = device_open,
  .unlocked_ioctl = device_ioctl,
  .read = device_read,
  .write = device_write,
  .release = device_release,
};

static int __init my_init(void)
{
    int rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &fops);
    if(rc < 0)
    {
        printk(KERN_ALERT"%s registraion failed for %d\n",
               DEVICE_FILE_NAME,
               MAJOR_NUM);
        return rc;
    }
    printk(KERN_INFO"message_slot: registered major number %d\n", MAJOR_NUM);
    return 0;
}

void slot_delete(struct slot* s)
{
    if (!s) return;
    slot_delete(s->next); // otherwise we'll lose the pointer
    kfree(s);
}

static void __exit my_cleanup(void)
{
    slot_delete(begin->next); // begin is sentinel - allocated on stack
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(my_init);
module_exit(my_cleanup);
