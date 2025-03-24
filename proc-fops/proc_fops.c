#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>

// proc entry filename
#define PROC_FILENAME "buffer_file"

// our struct to store buffer data
struct buffer_data {
    char *data;
    size_t length;
    struct list_head list;
};

static LIST_HEAD(buffer_list);
static DEFINE_MUTEX(mtx);
static struct proc_dir_entry *proc_entry;


// read callback func
static ssize_t proc_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    struct buffer_data *buffer;

    char* temp_buffer;
    size_t len = 0;

    // if all data provided, exit
    if (*offset > 0) {
        return 0;
    }

    // temp buffer to hold all the data
    temp_buffer = kzalloc(count, GFP_KERNEL);
    if (!temp_buffer) return -ENOMEM;

    mutex_lock(&mtx);
    // traverse list and copy data to temp buffer
    list_for_each_entry(buffer, &buffer_list, list) {
        if (len + buffer->length >= count) 
            break;
        memcpy(temp_buffer + len, buffer->data, buffer->length);
        len += buffer->length;
    }
    mutex_unlock(&mtx);

    // copy all data to user space now 
    if (copy_to_user(user_buffer, temp_buffer, len)) {
        kfree(temp_buffer);
        return -EFAULT;
    }

    kfree(temp_buffer);

    *offset += len;
    return len; 

}


// write callback func
static ssize_t proc_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    struct buffer_data *buffer;
    
    buffer = kmalloc(sizeof(struct buffer_data), GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;
    
    buffer->data = kmalloc(count, GFP_KERNEL);
    if (!buffer->data) {
        kfree(buffer);
        return -ENOMEM;
    }
    
    // copy to buffer from user space
    if (copy_from_user(buffer->data, user_buffer, count)) {
        kfree(buffer->data);
        kfree(buffer);
        return -EFAULT;
    }
    
    buffer->length = count;
    
    // add new data to the list
    mutex_lock(&mtx);
    list_add_tail(&buffer->list, &buffer_list);
    mutex_unlock(&mtx);
    
    *offset += count;
    return count;
}


static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};


// module init func
static int __init proc_init(void) {
    proc_entry = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);
    if (!proc_entry) {
        printk(KERN_ALERT "Failed to create proc entry\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Kernel module initialized successfully.\n");
    return 0;
}

// module cleanup
static void __exit proc_exit(void)
{
    struct buffer_data *buffer, *temp;
    
    proc_remove(proc_entry);
    
    mutex_lock(&mtx);
    list_for_each_entry_safe(buffer, temp, &buffer_list, list) { // free list memory
        list_del(&buffer->list);
        kfree(buffer->data);
        kfree(buffer);
    }
    mutex_unlock(&mtx);
    
    printk(KERN_INFO "Kernel module cleaned up successfully.\n");
}

module_init(proc_init);
module_exit(proc_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adilet Majit");
MODULE_DESCRIPTION("Kernel module for proc read/write file ops");
