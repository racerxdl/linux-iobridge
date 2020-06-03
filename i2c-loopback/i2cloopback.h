// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "i2c-loopback"
#define MSG_BUFFER_LEN 15
#define MAJOR_NUMBER 500
#define LOOP_I2C_XFER_TIMEOUT (msecs_to_jiffies(5000))

struct loopback_msg {
  __u16 addr;   /* slave address */
  __u16 flags;  /* see above for flag definitions */
  __u16 len;    /* msg length */
};

#define IOCTL_GET_I2C_MSG  0x1000 // _IOR(IOC_MAGIC, 0x01, sizeof(struct loopback_msg))
#define IOCTL_COMMIT_MSG   0x2000 // _IO(IOC_MAGIC, 0x02)

#define IOCTL_RET_SUCCESS  0
#define IOCTL_RET_NO_SRV   1
#define IOCTL_RET_NO_MSG   2
#define IOCTL_RET_FAIL     3



//struct i2c_msg {
//__u16 addr; /* slave address */
//__u16 flags; /* see above for flag definitions */
//__u16 len; /* msg length */
//__u8 *buf; /* pointer to msg data */
//};

// Defines for struct i2c_msg - flags field
// #define I2C_M_RD 0x0001 /* read data, from slave to master */
// #define I2C_M_TEN 0x0010 /* this is a ten bit chip address */
// #define I2C_M_RECV_LEN 0x0400 /* length will be first received byte */
// #define I2C_M_NO_RD_ACK 0x0800 /* if I2C_FUNC_PROTOCOL_MANGLING */
// #define I2C_M_IGNORE_NAK 0x1000 /* if I2C_FUNC_PROTOCOL_MANGLING */
// #define I2C_M_REV_DIR_ADDR 0x2000 /* if I2C_FUNC_PROTOCOL_MANGLING */
// #define I2C_M_NOSTART 0x4000 /* if I2C_FUNC_NOSTART */
// #define I2C_M_STOP 0x8000 /* if I2C_FUNC_PROTOCOL_MANGLING */
//
struct loopback_i2c_dev {
  struct i2c_adapter adapter;
  struct completion msg_complete;
  struct device *dev;
  struct i2c_msg *msg;
  int msg_err;

  // void __iomem *base;
  // struct i2c_msg *msg;
  // size_t msg_len;
  // struct device *dev;
  // struct i2c_adapter adapter;
  // struct clk *i2c_clk;
  // u32 bus_clk_rate;
  // u8 *buf;
  // u32 fifo_size;
  // u32 isr_mask;
  // u32 isr_status;
  // spinlock_t lock;  /* IRQ synchronization */
  // struct mutex isr_mutex;
};

extern struct mutex msg_mutex;
extern struct loopback_i2c_dev *i2cslave;

extern int process_message(struct i2c_msg *msg);

extern int register_i2cslave(void);
extern void unregister_i2cslave(void);
