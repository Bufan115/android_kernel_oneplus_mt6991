#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include "comm.h"
#include "memory.h"
#include "process.h"

#define DEVICE_NAME "wanbai"

char* genRandomString(void)
{
    static char string[10];
    int i, seed, random;
    
    for (i = 0; i < 5; i++)
    {
        get_random_bytes(&seed, sizeof(int));
        random = seed % 26;
        if (random < 0)
            random = random * -1;
        string[i] = 'a' + random;
    }
    string[5] = '\0';
    return string;
}

int dispatch_open(struct inode *node, struct file *file)
{
    printk("wanbai: device opened\n");
    return 0;
}

int dispatch_close(struct inode *node, struct file *file)
{
    printk("wanbai: device closed\n");
    return 0;
}

long dispatch_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg)
{
    COPY_MEMORY cm;
    MODULE_BASE mb;
    char name[0x100] = {0};

    switch (cmd) {
        case OP_READ_MEM:
            if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
                return -1;
            }
            if (read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
                return -1;
            }
            break;
        case OP_WRITE_MEM:
            if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
                return -1;
            }
            if (write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
                return -1;
            }
            break;
        case OP_MODULE_BASE:
            if (copy_from_user(&mb, (void __user*)arg, sizeof(mb)) != 0 || 
                copy_from_user(name, (void __user*)mb.name, sizeof(name)-1) !=0) {
                return -1;
            }
            mb.base = get_module_base(mb.pid, name);
            if (copy_to_user((void __user*)arg, &mb, sizeof(mb)) !=0) {
                return -1;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations dispatch_functions = {
    .owner  = THIS_MODULE,
    .open   = dispatch_open,
    .release = dispatch_close,
    .unlocked_ioctl = dispatch_ioctl,
};

static int __init driver_entry(void)
{
    int ret;
    const char *devicename = DEVICE_NAME;
    
    printk("wanbai: driver loading\n");
    
    // 使用随机设备名
    devicename = genRandomString();
    printk("wanbai: device name: %s\n", devicename);
    
    // 注册字符设备
    ret = register_chrdev(0, devicename, &dispatch_functions);
    if (ret < 0) {
        printk("wanbai: failed to register device\n");
        return ret;
    }
    
    printk("wanbai: driver loaded successfully\n");
    return 0;
}

static void __exit driver_unload(void)
{
    printk("wanbai: driver unloaded\n");
}

module_init(driver_entry);
module_exit(driver_unload);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanbai");
MODULE_DESCRIPTION("Memory access driver");
