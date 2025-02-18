# Linux Kernel Drivers

This repository contains Linux kernel driver projects. Each project is structured as a separate module that can be built and inserted into the Linux kernel.

More drivers to be added along my learning journey. 

## 📂 Projects

### Key Logger
**Description:**
A Linux kernel module that captures keyboard input events and logs keystrokes.

🔗 [Key Logger Source Code](./kernel_keylogger/)

---

### Virtual Framebuffer Driver
**Description:**
A Linux kernel module that implements a virtual framebuffer device (`/dev/fb1`). This driver allows software rendering to a framebuffer without physical display hardware.

🔗 [Virtual Framebuffer Driver Source Code](./virtual_framebuffer/)

---

### Virtual LED Driver
**Description:**
A simple virtual LED device driver `/dev/vled` for Linux. It simulates an LED which can be turned on and off through simple file operations.

🔗 [Virtual LED Driver Source Code](./virtual-led/)

---

### Network Blocker LSM
**Description:**
A Linux Security Module that intercepts and blocks network access for targeted processes by hooking into security-related socket functions.

🔗 [Source code](./network-blocker/)

---

## 🛠️ Setup & Compilation
To build and install each module:
```bash
make
sudo insmod <module_name>.ko
```
To remove the module:
```bash
sudo rmmod <module_name>
```
Check kernel logs for debugging:
```bash
dmesg | tail -n 20
```

---

## 📜 License
This repository is licensed under the GPL-2.0 License
