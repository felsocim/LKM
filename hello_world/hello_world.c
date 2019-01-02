#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("WTF-tp1");
MODULE_AUTHOR("Marek Felsoci");
MODULE_DESCRIPTION("Hello world!");
MODULE_VERSION("1");

static int hello_init(void) {
  printk(KERN_INFO "Hello world!\n");
  return 0;
}

static void hello_exit(void) {
  printk(KERN_INFO "Bye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
