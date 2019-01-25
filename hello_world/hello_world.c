#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

// Specify module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marek Felsoci");
MODULE_DESCRIPTION("Hello world!");
MODULE_VERSION("1.0");

// Function called on module initialization (when the module is loaded)
static int hello_init(void) {
  printk(KERN_INFO "Hello world!\n");
  return 0;
}

// Function called when the module is unloaded from the system
static void hello_exit(void) {
  printk(KERN_INFO "Bye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
