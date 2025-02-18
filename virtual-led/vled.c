#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "vled"
#define CLASS_NAME "virtual"


static int major_number;
static struct class* vledClass = NULL;
static struct device* vledDevice = NULL;
static bool led_state = false; 


static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_read(struct file*, char*, size_t, loff_t*);
static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init vled_init(void) {
    printk(KERN_INFO "VLED: Initializing the Virtual LED device\n");
    

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "VLED failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "VLED: registered correctly with major number %d\n", major_number);
    
    // register the device class
    vledClass = class_create(CLASS_NAME);
    if (IS_ERR(vledClass)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(vledClass);
    }
    printk(KERN_INFO "VLED: device class registered correctly\n");
    
    // register the device driver
    vledDevice = device_create(vledClass, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(vledDevice)) {
        class_destroy(vledClass);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(vledDevice);
    }
    printk(KERN_INFO "VLED: device class created correctly\n");
    
    return 0;
}

static void __exit vled_exit(void) {
    device_destroy(vledClass, MKDEV(major_number, 0));
    class_unregister(vledClass);
    class_destroy(vledClass);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "VLED: Unloading Virtual LED device!\n");
}


static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "VLED: Device has been opened\n");
    return 0;
}

// when user reads from device
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    char temp[3]; // "0\n" plus null terminator
    int error_count = 0;
    

    if (led_state) {
        temp[0] = '1';
    } else {
        temp[0] = '0';
    }
    temp[1] = '\n';
    temp[2] = 0;
    

    error_count = copy_to_user(buffer, temp, 2);
    
    if (error_count == 0) {
        printk(KERN_INFO "VLED: Sent %d characters to the user\n", 2);
        return 2;
    } else {
        printk(KERN_INFO "VLED: Failed to send %d characters to the user\n", error_count);
        return -EFAULT;
    }
}

// write to device
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    if (len > 0) {
        char value;
        if (copy_from_user(&value, buffer, 1)) {
            return -EFAULT;
        }
        
        if (value == '1') {
            led_state = true;
            printk(KERN_INFO "VLED: LED turned ON\n");
        } else if (value == '0') {
            led_state = false;
            printk(KERN_INFO "VLED: LED turned OFF\n");
        } else {
            printk(KERN_INFO "VLED: Invalid input. Use '1' to turn ON or '0' to turn OFF\n");
        }
    }
    return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "VLED: Device successfully closed\n");
    return 0;
}

module_init(vled_init);
module_exit(vled_exit);


//meta
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adilet Majit");
MODULE_DESCRIPTION("A simple virtual LED driver");
