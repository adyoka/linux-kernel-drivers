# Keyboard Logger Driver for Linux

### Overview
This is a simple Linux kernel module that logs keyboard events by capturing key presses. The module registers a keyboard notifier callback, processes key events, and writes them to a log file asynchronously using a work queue.


### Requirements
- Linux kernel headers installed
- Root privileges to load and unload the kernel module

### Installation
1. Clone the repository:
2. Build the kernel module:
   ```bash
   make
   ```
3. Load the module:
   ```bash
   sudo insmod keylogger.ko
   ```
4. Verify the module is loaded:
   ```bash
   lsmod | grep keylogger
   ```
5. View the logs:
   ```bash
   cat /var/log/keylog.txt
   ```
   

### Uninstallation
1. Remove the kernel module:
   ```bash
   sudo rmmod keylogger
   ```
2. Clean up generated files:
   ```bash
   make clean
   ```

### License
This project is licensed under the GPL-2.0 License.


