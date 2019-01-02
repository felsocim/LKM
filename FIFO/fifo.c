#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* file_operations, etc. */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/cdev.h>  /** character device **/
#include <linux/device.h>  /** for sys device registration **/
#include <linux/sched.h>  /** for WaitQ **/
#include <linux/uaccess.h> /* copy_from/to_user */

// GPL required for class_create/device_create/etc.
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marek");
MODULE_DESCRIPTION("fifo");
MODULE_VERSION("0.0.1");

// functions pre-declaration
int m_open(struct inode * inode, struct file * filp);
int m_release(struct inode * inode, struct file * filp);
ssize_t m_read(struct file * filp, char * buf, size_t count, loff_t * f_pos);
ssize_t m_write(struct file * filp, const char * buf, size_t count, loff_t * f_pos);
void m_exit(void);
int m_init(void);

// Structure that declares the usual file access functions
struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = m_read,
  .write = m_write,
  .open = m_open,
  .release = m_release
};

// Declaration of the module init and exit functions
module_init(m_init);
module_exit(m_exit);

// Global variables of the driver
static int m_major;
static struct class * basicDriverClass;
static int nbWriters = 0;

// Buffer to store data
static char * m_buffer;
static int m_debut, m_fin;

#define BUF_SIZE 256

DECLARE_WAIT_QUEUE_HEAD(WrWaitQ);
DECLARE_WAIT_QUEUE_HEAD(RdWaitQ);

int m_init(void) {
  // a major number will be dynamically allocated here
  m_major = register_chrdev(0, "fifo", &fops);

  if(m_major < 0) {
    printk(KERN_ALERT "FIFO: Error in allocating device\n");
    return -1;    
  }
  
  printk(KERN_INFO "FIFO: Kernel assigned major number is %d.\n", m_major);

  // add the driver class to /sys/class/fifodrv
  if((basicDriverClass = class_create(THIS_MODULE, "fifodrv")) == NULL) {
    printk(KERN_ALERT "FIFO: Error in creating device class\n");
    goto fail1;
  }

  // add the driver file to /dev/fifo -- here
  if(device_create(basicDriverClass, NULL, MKDEV(m_major,0), NULL, "fifo") == NULL) {
    printk(KERN_ALERT "FIFO: Error in creating device /dev/fifo\n");
    goto fail2;
  }

  // memory for the buffer
  m_buffer = kmalloc(BUF_SIZE, GFP_KERNEL); 
  if(!m_buffer) {
    printk(KERN_ALERT "registering fifo: cannot allocate memory\n");
    goto fail3;
  }

  m_debut = 0;
  m_fin = 0;
  printk(KERN_INFO "FIFO: driver created successfully, device is /dev/fifo.\n");

  return(0);
  
  // errors: free the allocated resources in case of failure!
  fail3:
    device_destroy(basicDriverClass, MKDEV(m_major, 0));

  fail2:
    class_destroy(basicDriverClass);

  fail1:
    unregister_chrdev(m_major, "fifo");

  return(-ENOMEM);
}

void m_exit(void) {
  device_destroy(basicDriverClass, MKDEV(m_major, 0));
  class_destroy(basicDriverClass);
  unregister_chrdev(m_major, "fifo");
  kfree(m_buffer);
}

int m_open(struct inode * inode, struct file * filp) {
  try_module_get(THIS_MODULE);
  if(filp->f_mode & FMODE_WRITE) {
    nbWriters++;
  }
  return 0;
}

int m_release(struct inode * inode, struct file * filp) {
  module_put(THIS_MODULE);
  if(filp->f_mode & FMODE_WRITE) {
    nbWriters--;
  }
  return 0;
}

ssize_t m_read(struct file * filp, char * buf, size_t count, loff_t * f_pos) {
  int left = m_fin - m_debut;
  
  while(left < 1) {
    if(nbWriters < 1) {
      return 0;
    }	  
    wait_event_interruptible(RdWaitQ, left > 0);
  }
  
  copy_to_user(buf, &m_buffer[m_debut], 1);
  
  m_debut++;
  m_debut = m_debut % BUF_SIZE;
  
  (* f_pos)++;
  
  wake_up(&WrWaitQ);
  
  return 1;
}

ssize_t m_write(struct file * filp, const char * buf, size_t count, loff_t * f_pos) {
  while(m_debut == (m_fin + 1) % BUF_SIZE) {
    // buffer plein... attendre que quelqu'un lise le buffer
    wait_event_interruptible(WrWaitQ, !(m_debut == (m_fin + 1) % BUF_SIZE));
  }
  
  copy_from_user(&m_buffer[m_fin], buf, 1);
  
  m_fin++;
  m_fin = m_fin % BUF_SIZE;
  
  wake_up(&RdWaitQ);
  
  return 1;
}
