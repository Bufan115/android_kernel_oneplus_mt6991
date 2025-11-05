#include <linux/module.h>
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include "comm.h"
#include "memory.h"
#include "process.h"
//#include "verify.h"

#define DEVICE_NAME "wanbai"

void get_random_bytes(void *buf, int nbytes);
char* genRandomString()
{
	static char string[10];
	int i,seed,random;
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
	return 0;
}

int dispatch_close(struct inode *node, struct file *file)
{
	return 0;
}

long dispatch_ioctl(struct file* const file, unsigned int const cmd, unsigned long const arg)
{
	static COPY_MEMORY cm;
	static MODULE_BASE mb;
	static char name[0x100] = {0};
	/*static char key[0x100] = {0};
	static bool is_verified = false;
	if(cmd == OP_INIT_KEY && !is_verified) {
		if (copy_from_user(key, (void __user*)arg, sizeof(key)-1) != 0) {
			return -1;
		}
		is_verified = init_key(key, sizeof(key));
	}
	if(is_verified == false) {
		return -1;
	}*/
	switch (cmd) {
		case OP_READ_MEM:
			{
				if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
					return -1;
				}
				if (read_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
					return -1;
				}
			}
			break;
		case OP_WRITE_MEM:
			{
				if (copy_from_user(&cm, (void __user*)arg, sizeof(cm)) != 0) {
					return -1;
				}
				if (write_process_memory(cm.pid, cm.addr, cm.buffer, cm.size) == false) {
					return -1;
				}
			}
			break;
		case OP_MODULE_BASE:
			{
				if (copy_from_user(&mb, (void __user*)arg, sizeof(mb)) != 0 
				|| copy_from_user(name, (void __user*)mb.name, sizeof(name)-1) !=0) {
					return -1;
				}
				mb.base = get_module_base(mb.pid, name);
				if (copy_to_user((void __user*)arg, &mb, sizeof(mb)) !=0) {
					return -1;
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

struct file_operations dispatch_functions = {
	.owner  = THIS_MODULE,
	.open	= dispatch_open,
	.release = dispatch_close,
	.unlocked_ioctl = dispatch_ioctl,
};

dev_t dev;
struct cdev hack_cdev;
struct class *hack_class;
const char *devicename;

static int __init driver_entry(void)
{
	printk("[+] driver_entry");
	devicename = DEVICE_NAME;
	devicename = genRandomString();//注释此行关闭随机驱动
   	alloc_chrdev_region(&dev, 0, 1, devicename);
	cdev_init(&hack_cdev, &dispatch_functions);
	cdev_add(&hack_cdev, dev, 1);
	hack_class = class_create(THIS_MODULE, devicename);
	device_create(hack_class, NULL, dev, NULL, devicename);
	list_del_init(&THIS_MODULE->list);// 摘除链表，/proc/modules 中不可见。
	kobject_del(&THIS_MODULE->mkobj.kobj);// 摘除kobj，/sys/modules/中不可见。
	return 0;
}

static void __exit driver_unload(void)
{
	printk("[+] driver_unload");
	device_destroy(hack_class, dev);
	class_destroy(hack_class);
	cdev_del(&hack_cdev);
	unregister_chrdev_region(dev, 1);
}

module_init(driver_entry);
module_exit(driver_unload);

MODULE_DESCRIPTION("Linux Kernel.");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanbai");
//by----时光弟弟开源