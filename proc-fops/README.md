# Kernel module for proc file ops

A simple Linux kernel module that creates a proc file entry to store and retrieve data.

### Usage:

1. Compile:
   ```
   make
   ```

2. Load the module:
   ```
   sudo insmod proc_fops.ko
   ```

3. Write data
   ```
   echo "Hello!" > /proc/buffer_file
   ```

4. Read data
   ```
   cat /proc/buffer_file
   ```

5. Unload module
   ```
   sudo rmmod proc_fops.ko
   ```