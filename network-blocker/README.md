# Linux Security Module: Blocking Network of Targeted Processes

### Overview
This project implements a Linux Security Module (LSM) to monitor and block network activity of specific processes in real-time. The module uses KProbes and hooks into security-related socket functions and prevents network access for designated processes.

Following ideas from this [guide]{https://medium.com/@emanuele.santini.88/creating-a-linux-security-module-with-kprobes-blocking-network-of-targeted-processes-4046f50290f5}

### Build & Load Module
```sh
make
sudo insmod lsm_kprobe.ko
```

Check logs:
```sh
dmesg | grep "Blocking"
```

Now try to download something with `wget`

Unload Module
```sh
sudo rmmod lsm_kprobe
```