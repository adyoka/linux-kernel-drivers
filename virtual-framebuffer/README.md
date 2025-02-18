# Virtual Framebuffer Driver for Linux

A simple virtual framebuffer driver that creates a virtual display device capable of text rendering. This kernel module emulates a physical graphics card by implementing the Linux framebuffer interface.

## Overview

This project creates a virtual framebuffer that applications can render to without requiring physical display hardware. It implements core framebuffer functionality, including:

- 32-bit RGBA color depth (configurable)
- Memory-mapped framebuffer access
- Platform device integration for clean kernel abstraction
- Basic text rendering capabilities
- Standard framebuffer ioctl support


## Building and Installing

1. Clone the repository:
   ```
   git clone https://github.com/adyoka/virtual-framebuffer.git
   cd virtual-framebuffer
   ```

2. Compile the module:
   ```
   make
   ```

3. Load the module:
   ```
   sudo insmod virtual_fb.ko
   ```

4. Check that it loaded successfully:
   ```
   dmesg | grep virtualfb
   ```

## Testing

Once loaded, the framebuffer device should be available (typically as `/dev/fb1`).

Basic test using `fbset`:
```
sudo apt install fbset
sudo fbset -fb /dev/fb1
```

You can view the framebuffer content using various tools:
```
fbgrab screenshot.png
```

Or use a framebuffer viewer:
```
sudo apt-get install fbv
fbv /path/to/image.png
```


## Configuration

The driver has several configurable parameters in `virtual_fb.c`:

- `VIRTUAL_WIDTH`: Width of the virtual display (default: 800)
- `VIRTUAL_HEIGHT`: Height of the virtual display (default: 600)
- `VIRTUAL_BPP`: Bits per pixel (default: 32)
- `FONT_NAME`: Font name for char rendering (default: VGA8x8)

To change these settings, modify the defines in the source code and recompile.

## Implementation Details

The core of this implementation is in the platform driver model, with:

1. A memory-mapped framebuffer accessible to userspace
2. Standard framebuffer operations (fb_ops)
3. Color format handling for 32-bit RGBA
4. Text rendering support using an 8x8 font
5. Integration with Linux's platform device subsystem
