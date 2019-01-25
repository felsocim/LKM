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
MODULE_AUTHOR("Marek Felsoci");
MODULE_DESCRIPTION("fifo");
MODULE_VERSION("1.0");

// Function pre-declarations
int m_open(struct inode * inode, struct file * filp);
int m_release(struct inode * inode, struct file * filp);
ssize_t m_read(struct file * filp, char * buf, size_t count, loff_t * f_pos);
ssize_t m_write(struct file * filp, const char * buf, size_t count, loff_t * f_pos);
void m_exit(void);
int m_init(void);

// Structure that declares the usual device file access functions
struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = m_read,
  .write = m_write,
  .open = m_open,
  .release = m_release
};

// Declaration of the module initialization and exit functions
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

// Declaration of waiting queues
DECLARE_WAIT_QUEUE_HEAD(WrWaitQ);
DECLARE_WAIT_QUEUE_HEAD(RdWaitQ);

// Function called when the module is loaded to the system
int m_init(void) {
  // New device definition
  // - a major number will be dynamically allocated here
  m_major = register_chrdev(0, "fifo", &fops);

  if(m_major < 0) {
    printk(KERN_ALERT "FIFO: Error registering new device!\n");
    return -1;    
  }
  
  printk(KERN_INFO "FIFO: Kernel assigned major number is %d.\n", m_major);

  // Creating device's driver class
  // - add the driver class to /sys/class/fifodrv
  if((basicDriverClass = class_create(THIS_MODULE, "fifodrv")) == NULL) {
    printk(KERN_ALERT "FIFO: Error creating device class '/sys/class/fifodrv'!\n");
    goto fail1;
  }

  // Adding the device's driver file to /dev/fifo
  if(device_create(basicDriverClass, NULL, MKDEV(m_major,0), NULL, "fifo") == NULL) {
    printk(KERN_ALERT "FIFO: Error creating device '/dev/fifo'!\n");
    goto fail2;
  }

  // Alocate memory for the device's buffer
  m_buffer = kmalloc(BUF_SIZE, GFP_KERNEL); 
  if(!m_buffer) {
    printk(KERN_ALERT "FIFO: Memory allocation for the device buffer failed!\n");
    goto fail3;
  }

  // Initialize buffer's start and end positions to 0
  m_debut = 0;
  m_fin = 0;
  
  printk(KERN_INFO "FIFO: Device driver created successfully, device is /dev/fifo.\n");

  return(0);
  
  // Error handling: free the allocated resources in case of failure!
  fail3:
    device_destroy(basicDriverClass, MKDEV(m_major, 0));

  fail2:
    class_destroy(basicDriverClass);

  fail1:
    unregister_chrdev(m_major, "fifo");

  return(-ENOMEM);
}

// Destroys the device and frees the memory used.
void m_exit(void) {
  device_destroy(basicDriverClass, MKDEV(m_major, 0));
  class_destroy(basicDriverClass);
  unregister_chrdev(m_major, "fifo");
  kfree(m_buffer);
}

// Prepares the device for a new writer.
int m_open(struct inode * inode, struct file * filp) {
  try_module_get(THIS_MODULE);
  if(filp->f_mode & FMODE_WRITE) {
    nbWriters++;
  }
  return 0;
}

// Detaches a registered writer from the device.
int m_release(struct inode * inode, struct file * filp) {
  module_put(THIS_MODULE);
  if(filp->f_mode & FMODE_WRITE) {
    nbWriters--;
  }
  return 0;
}

// Circuralry reads characters from the device and sends it to the user until the buffer becomes empty, then waits until there are new characters in the buffer or exists if there is no more characters and no more attached writers.
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

// Reads characters from the user and writes them to the buffer until it becomes full, then waits until the 'm_read' function reads the characters from the buffer and continues till the end of the input stream. 
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
