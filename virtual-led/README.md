# Virtual LED Device Driver

This project implements a simple virtual LED device driver for Linux. It creates a character device `/dev/vled` that simulates an LED which can be turned on and off through simple file operations.


## Building and Loading the Driver

1. Clone this repository:

2. Build the driver:
   ```
   make
   ```
   This will compile the driver into a kernel module named `vled.ko`.

3. To load the driver, run:
```
sudo insmod vled.ko
```

You can verify that the driver has been loaded by checking kernel messages:
```
dmesg | tail
```

You should see messages like:
```
VLED: Initializing the Virtual LED device
VLED: registered correctly with major number XXX
VLED: device class registered correctly
VLED: device class created correctly
```


Once loaded, you can interact with the virtual LED through the `/dev/vled` device.

```
echo 1 > /dev/vled
echo 0 > /dev/vled
```

```
cat /dev/vled
```


4. To remove the driver from the kernel:
```
sudo rmmod vled
```

## License

This project is licensed under the GPL v2.
