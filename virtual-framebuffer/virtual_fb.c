#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/console.h>   // find_font()
#include <linux/font.h>      // font structures



#define VIRTUAL_FB_NAME "virtualfb"
#define VIRTUAL_WIDTH 800
#define VIRTUAL_HEIGHT 600
#define VIRTUAL_BPP 32  // 32 bpp
#define VIRTUAL_BUFFER_SIZE (VIRTUAL_WIDTH * VIRTUAL_HEIGHT * (VIRTUAL_BPP / 8))

#define FONT_NAME "VGA8x8"

// global vars
static void __iomem *framebuffer_memory;
static u32 pseudo_palette[16];
static struct fb_info *virtual_fb_info;
static struct platform_device *virtual_fb_device;


static int virtual_fb_set_par(struct fb_info *info);
static int virtual_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int virtual_fb_mmap(struct fb_info *info, struct vm_area_struct *vma);
static int virtual_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info);
static int virtual_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int __init virtual_fb_probe(struct platform_device *dev);
static int virtual_fb_remove(struct platform_device *dev);


static struct fb_ops virtual_fb_ops = {
    .owner          = THIS_MODULE,
    .fb_check_var   = virtual_fb_check_var,
    .fb_set_par     = virtual_fb_set_par,
    .fb_mmap        = virtual_fb_mmap,
    .fb_setcolreg   = virtual_fb_setcolreg,
    .fb_fillrect    = cfb_fillrect,
    .fb_copyarea    = cfb_copyarea,
    .fb_imageblit   = cfb_imageblit,
    .fb_pan_display = virtual_fb_pan_display,
};


static struct platform_driver virtual_fb_driver = {
    .probe  = virtual_fb_probe,
    .remove = virtual_fb_remove,
    .driver = {
        .name = VIRTUAL_FB_NAME,
    },
};

// TODO
static int virtual_fb_set_par(struct fb_info *info)
{
    return 0;
}

static int virtual_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
    if (var->xres != VIRTUAL_WIDTH || var->yres != VIRTUAL_HEIGHT)
        return -EINVAL;
    if (var->bits_per_pixel != VIRTUAL_BPP)
        return -EINVAL;

    var->xres_virtual = var->xres;
    var->yres_virtual = var->yres;

    var->red.length = 8;
    var->red.offset = 16;
    var->green.length = 8;
    var->green.offset = 8;
    var->blue.length = 8;
    var->blue.offset = 0;
    var->transp.length = 8;
    var->transp.offset = 24;

    return 0;
}

static int virtual_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
    unsigned long start = vma->vm_start;
    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long pfn;

    if (offset + size > VIRTUAL_BUFFER_SIZE)
        return -EINVAL;

    pfn = (virt_to_phys(framebuffer_memory + offset)) >> PAGE_SHIFT;
    
    if (remap_pfn_range(vma, start, pfn, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static int virtual_fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int transp, struct fb_info *info)
{
    if (regno >= 16)
        return -EINVAL;

    // convert RGB888 to RGBA8888
    pseudo_palette[regno] = ((transp & 0xFF) << 24) | ((red & 0xFF) << 16) |
                             ((green & 0xFF) << 8) | (blue & 0xFF);
    return 0;
}

// TODO
static int virtual_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
    return 0;
}

// helper
static void virtual_fb_clear(u32 color)
{
    int i;
    u32 *pixel = (u32 *)framebuffer_memory;

    for (i = 0; i < (VIRTUAL_WIDTH * VIRTUAL_HEIGHT); i++)
        *pixel++ = color;
}

// draw a char
static void virtual_fb_draw_char(int x, int y, char c, u32 color)
{
    int i, j;
    const struct font_desc *font;
    const u8 *glyph;

    font = find_font(FONT_NAME);  // find font dynamically
    if (!font || !font->data) {
        printk(KERN_ERR "Failed to find VGA8x8 font\n");
        return;
    }

    glyph = font->data + (c * font->height);

    for (i = 0; i < font->height; i++) {
        u32 *pixel = (u32 *)(framebuffer_memory + ((y + i) * VIRTUAL_WIDTH + x) * 4);
        for (j = 0; j < font->width; j++) {
            if (glyph[i] & (1 << (7 - j)))  // check if pixel should be drawn
                *pixel = color;
            pixel++;
        }
    }
}

// draw text string
static void virtual_fb_draw_text(int x, int y, const char *text, u32 color)
{
    int i = 0;
    while (text[i]) {
        virtual_fb_draw_char(x + (i * 8), y, text[i], color);
        i++;
    }
}


static int __init virtual_fb_probe(struct platform_device *dev)
{
    int ret = 0;

    
    framebuffer_memory = vmalloc(VIRTUAL_BUFFER_SIZE);
    if (!framebuffer_memory) {
        pr_err("Failed to allocate framebuffer memory\n");
        return -ENOMEM;
    }
    memset(framebuffer_memory, 0, VIRTUAL_BUFFER_SIZE);

    
    virtual_fb_info = framebuffer_alloc(sizeof(u32) * 16, &dev->dev);
    if (!virtual_fb_info) {
        pr_err("Failed to allocate fb_info\n");
        vfree(framebuffer_memory);
        return -ENOMEM;
    }

    virtual_fb_info->screen_base = framebuffer_memory;
    virtual_fb_info->fbops = &virtual_fb_ops;
    virtual_fb_info->pseudo_palette = pseudo_palette;

    virtual_fb_info->fix.smem_start = virt_to_phys(framebuffer_memory);
    virtual_fb_info->fix.smem_len = VIRTUAL_BUFFER_SIZE;
    virtual_fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
    virtual_fb_info->fix.visual = FB_VISUAL_TRUECOLOR;
    virtual_fb_info->fix.line_length = VIRTUAL_WIDTH * (VIRTUAL_BPP / 8);
    strcpy(virtual_fb_info->fix.id, VIRTUAL_FB_NAME);

    virtual_fb_info->var.xres = VIRTUAL_WIDTH;
    virtual_fb_info->var.yres = VIRTUAL_HEIGHT;
    virtual_fb_info->var.xres_virtual = VIRTUAL_WIDTH;
    virtual_fb_info->var.yres_virtual = VIRTUAL_HEIGHT;
    virtual_fb_info->var.bits_per_pixel = VIRTUAL_BPP;
    virtual_fb_info->var.activate = FB_ACTIVATE_NOW;
    virtual_fb_info->var.height = -1;
    virtual_fb_info->var.width = -1;

    virtual_fb_info->var.red.length = 8;
    virtual_fb_info->var.red.offset = 16;
    virtual_fb_info->var.green.length = 8;
    virtual_fb_info->var.green.offset = 8;
    virtual_fb_info->var.blue.length = 8;
    virtual_fb_info->var.blue.offset = 0;
    virtual_fb_info->var.transp.length = 8;
    virtual_fb_info->var.transp.offset = 24;

    ret = register_framebuffer(virtual_fb_info);
    if (ret < 0) {
        pr_err("Failed to register framebuffer device\n");
        framebuffer_release(virtual_fb_info);
        vfree(framebuffer_memory);
        return ret;
    }

    // dark blue 
    virtual_fb_clear(0xFF000040);

    // draw some text
    virtual_fb_draw_text(100, 100, "Hello World!", 0xFFFFFFFF);

    pr_info("Virtual framebuffer initialized: %dx%d, %d bpp\n",
            VIRTUAL_WIDTH, VIRTUAL_HEIGHT, VIRTUAL_BPP);
    printk(KERN_INFO "Virtual framebuffer initialized: %dx%d, %d bpp\n",
            VIRTUAL_WIDTH, VIRTUAL_HEIGHT, VIRTUAL_BPP)
    return 0;
}

static int virtual_fb_remove(struct platform_device *dev)
{
    unregister_framebuffer(virtual_fb_info);
    framebuffer_release(virtual_fb_info);
    vfree(framebuffer_memory);
    pr_info("Virtual framebuffer removed\n");
    return 0;
}


static int __init virtual_fb_init(void)
{
    printk(KERN_INFO "Virtual framebuffer module loaded.\n");

    int ret;

    virtual_fb_device = platform_device_register_simple(VIRTUAL_FB_NAME, 0, NULL, 0);
    if (IS_ERR(virtual_fb_device)) {
        pr_err("Failed to register platform device\n");
        return PTR_ERR(virtual_fb_device);
    }

    ret = platform_driver_register(&virtual_fb_driver);
    if (ret) {
        pr_err("Failed to register platform driver\n");
        platform_device_unregister(virtual_fb_device);
        return ret;
    }

    pr_info("Virtual framebuffer driver loaded\n");
    return 0;
}


static void __exit virtual_fb_exit(void)
{
    platform_driver_unregister(&virtual_fb_driver);
    platform_device_unregister(virtual_fb_device);
    pr_info("Virtual framebuffer driver unloaded\n");

    printk(KERN_INFO "Virtual framebuffer module unloaded.\n");
}

module_init(virtual_fb_init);
module_exit(virtual_fb_exit);


// meta
MODULE_AUTHOR("Adilet Majit");
MODULE_DESCRIPTION("Simple Virtual Framebuffer Driver");
MODULE_LICENSE("GPL v2");