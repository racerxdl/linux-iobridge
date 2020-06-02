// SPDX-License-Identifier: GPL-2.0-or-later

#include "i2cloopback.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucas Teske");
MODULE_DESCRIPTION("A I2C Loopback");
MODULE_VERSION("0.01");

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *, unsigned int, unsigned long);
static int major_num;
static int device_open_count = 0;
static __u8 *msg_ptr = NULL;
static int msg_len = 0;

struct mutex msg_mutex;

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
 .read = device_read,
 .write = device_write,
 .open = device_open,
 .unlocked_ioctl = device_ioctl,
 .release = device_release
};

static long device_ioctl(struct file *file,
     unsigned int ioctl_num,
     unsigned long ioctl_param)
{
  char *temp;
  struct loopback_msg msgbuff;
  long ret = -1;
  int res;
  int needsComplete = 0;

  mutex_lock(&msg_mutex);
  if (i2cslave == NULL) {
    ret = IOCTL_RET_NO_SRV;
  } else {
    // Process the IOCTL
    switch (ioctl_num) {
      case IOCTL_GET_I2C_MSG:
        if (i2cslave->msg == NULL) {
          // printk(KERN_DEBUG "User requested XFER MSG but no MSG pending\n");
          ret = IOCTL_RET_NO_MSG;
        } else {
          // printk(KERN_DEBUG "User requested XFER MSG\n");
          msgbuff.addr = i2cslave->msg->addr;
          msgbuff.flags = i2cslave->msg->flags;
          msgbuff.len = i2cslave->msg->len;
          ret = IOCTL_RET_SUCCESS;

          temp = (char *)ioctl_param;
          if (temp == NULL) {
            ret = IOCTL_RET_FAIL;
          } else {
            res = raw_copy_to_user(temp, &msgbuff, sizeof(msgbuff));
            if (res != 0) {
              ret = IOCTL_RET_FAIL;
            }
          }
        }
        break;
      case IOCTL_COMMIT_MSG:
        // printk(KERN_DEBUG "Received COMMIT\n");
        if (i2cslave->msg == NULL) {
          // printk(KERN_DEBUG "Received commit but no message pending\n");
          ret = IOCTL_RET_NO_MSG;
          break;
        }
        complete(&i2cslave->msg_complete);
        needsComplete = 1;
        i2cslave->msg_err = 0;
        ret = IOCTL_RET_SUCCESS;
        break;
    }
  }
  mutex_unlock(&msg_mutex);

  if (needsComplete) {
    needsComplete = 0;
  }

  return ret;
}

/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {
  int bytes_read = 0;

  mutex_lock(&msg_mutex);
  if (i2cslave != NULL && i2cslave->msg != NULL) {
    if (msg_ptr == NULL) {
      // First read
      msg_ptr = i2cslave->msg->buf;
      msg_len = i2cslave->msg->len;
    }

    if (len > msg_len) {
      len = msg_len;
    }

    while (len) {
     /* Buffer is in user data, not kernel, so you can’t just reference
     * with a pointer. The function put_user handles this for us */
     put_user(*(msg_ptr++), buffer++);
     len--;
     bytes_read++;
    }

    if (msg_ptr >= i2cslave->msg->buf + msg_len) {
      msg_ptr = NULL;
      msg_len = 0;
    }
  }
  mutex_unlock(&msg_mutex);

  return bytes_read;
}

/* Called when a process tries to write to our device */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
  int bytes_wrote = 0;

  mutex_lock(&msg_mutex);
  if (i2cslave != NULL && i2cslave->msg != NULL) {
    if (msg_ptr == NULL) {
      // First write
      msg_ptr = i2cslave->msg->buf;
      msg_len = i2cslave->msg->len;
    }

    if (len > msg_len) {
      len = msg_len;
    }

    while (len) {
     /* Buffer is in user data, not kernel, so you can’t just reference
     * with a pointer. The function put_user handles this for us */
     get_user(*(msg_ptr++), buffer++);
     len--;
     bytes_wrote++;
    }

    if (msg_ptr >= i2cslave->msg->buf + msg_len) {
      msg_ptr = NULL;
      msg_len = 0;
    }
  }
  mutex_unlock(&msg_mutex);

  return bytes_wrote;
}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file) {
  /* If device is open, return busy */
  if (device_open_count) {
    return -EBUSY;
  }
  // printk(KERN_DEBUG "Device Open\n");
  device_open_count++;
  try_module_get(THIS_MODULE);
  return 0;
}

/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file) {
  /* Decrement the open counter and usage count. Without this, the module would not unload. */
  device_open_count--;
  // printk(KERN_DEBUG "Device Close\n");
  module_put(THIS_MODULE);
  return 0;
}

static int __init i2cbridge_init(void) {
  int ret;

  mutex_init(&msg_mutex);

  ret = register_i2cslave();
  if (ret) {
    printk(KERN_ALERT "Could not register i2c device: %d\n", ret);
    return ret;
  }

  /* Try to register character device */
  ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &file_ops);
  if (ret < 0) {
    printk(KERN_ALERT "Could not register device: %d\n", ret);
    return ret;
  } else {
    major_num = MAJOR_NUMBER;
    printk(KERN_INFO DEVICE_NAME " module loaded with device major number %d\n", major_num);
    return 0;
  }
}

int process_message(struct i2c_msg *msg) {
  unsigned long time_left;
  if (!device_open_count) {
    return -ENXIO;
  }

  // printk(KERN_DEBUG "Received message");
  mutex_lock(&msg_mutex);
  reinit_completion(&i2cslave->msg_complete);
  i2cslave->msg = msg;
  i2cslave->msg_err = 0;
  mutex_unlock(&msg_mutex);

  time_left = wait_for_completion_timeout(&i2cslave->msg_complete,
            LOOP_I2C_XFER_TIMEOUT);

  mutex_lock(&msg_mutex);
  i2cslave->msg = NULL;
  msg_ptr = NULL;
  msg_len = 0;
  mutex_unlock(&msg_mutex);

  if (time_left == 0) {
    printk(KERN_DEBUG "Transaction timed out.\n");
    return -ETIMEDOUT;
  }

  // printk(KERN_DEBUG "Transaction result: %d\n", i2cslave->msg_err);
  return i2cslave->msg_err;
}

static void __exit i2cbridge_exit(void) {
/* Remember — we have to clean up after ourselves.
   Unregister the character device. */
 unregister_chrdev(major_num, DEVICE_NAME);
 unregister_i2cslave();
 printk(KERN_INFO "Goodbye, World!\n");
}


module_init(i2cbridge_init);
module_exit(i2cbridge_exit);