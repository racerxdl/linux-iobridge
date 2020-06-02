// SPDX-License-Identifier: GPL-2.0-or-later

#include "i2cloopback.h"

static int device_registered = 0;
struct loopback_i2c_dev *i2cslave = NULL;

static int
i2cloopback_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
  int i, res;
  struct loopback_i2c_dev *idev;
  idev = i2c_get_adapdata(adap);

  // dev_info(idev->dev, "Received %d messages to process\n", num);
  for (i = 0; i < num; i++) {
    // dev_info(idev->dev, "MSG(%d) Addr %02x Flags %04x Len %d\n", i, msgs[i].addr, msgs[i].flags, msgs[i].len);
    res = process_message(&msgs[i]);
    if (res < 0) {
      return res;
    }
  }
  return num;
}

static u32 i2cloopback_i2c_func(struct i2c_adapter *adap)
{
  return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm i2cloopback_i2c_algo = {
  .master_xfer = i2cloopback_i2c_xfer,
  .functionality = i2cloopback_i2c_func,
};

static int i2cloopback_probe(struct platform_device *pdev)
{
    int ret;
    struct loopback_i2c_dev *idev = NULL;
    printk(KERN_INFO DEVICE_NAME " probe\n");

    idev = devm_kzalloc(&pdev->dev, sizeof(*idev), GFP_KERNEL);
    if (!idev)
      return -ENOMEM;

    idev->dev = &pdev->dev;
    init_completion(&idev->msg_complete);
    i2c_set_adapdata(&idev->adapter, idev);

    strlcpy(idev->adapter.name, pdev->name, sizeof(idev->adapter.name));
    idev->adapter.owner = THIS_MODULE;
    idev->adapter.algo = &i2cloopback_i2c_algo;
    idev->adapter.dev.parent = &pdev->dev;
    idev->adapter.dev.of_node = pdev->dev.of_node;

    platform_set_drvdata(pdev, idev);

    ret = i2c_add_adapter(&idev->adapter);
    if (ret) {
      return ret;
    }

    mutex_lock(&msg_mutex);
    i2cslave = idev;
    mutex_unlock(&msg_mutex);

    dev_info(&pdev->dev, "I2C Bridge Device Probe complete\n");

    return 0;
}

static int i2cloopback_remove(struct platform_device *pdev)
{
  struct loopback_i2c_dev *idev = platform_get_drvdata(pdev);

  i2c_del_adapter(&idev->adapter);

  mutex_lock(&msg_mutex);
  i2cslave = NULL;
  mutex_unlock(&msg_mutex);

  dev_info(&pdev->dev, "I2C Bridge Device remove complete\n");

  return 0;
}

static void i2cloopback_release(struct device *dev) {

}

static struct resource i2cloopback_resources[] = {};


static struct platform_device i2cloopback_device = {
  .name   = "i2c-loopback-slave",
  .id   = 0,
  .num_resources  = ARRAY_SIZE(i2cloopback_resources),
  .resource = i2cloopback_resources,
  .dev = {
    .release = i2cloopback_release,
  }
};


static struct platform_driver i2cloopback_driver = {
  .probe = i2cloopback_probe,
  .remove = i2cloopback_remove,
  .driver = {
    .name = "i2c-loopback-slave",
  },
};


int register_i2cslave(void) {
  if (!device_registered) {
    int ret = platform_driver_register(&i2cloopback_driver);

    if (ret) {
      printk(KERN_ERR DEVICE_NAME " cannot register i2c slave driver: %d\n", ret);
      return ret;
    }

    ret = platform_device_register(&i2cloopback_device);

    if (ret) {
      platform_driver_unregister(&i2cloopback_driver);
      printk(KERN_ERR DEVICE_NAME " cannot register i2c slave device: %d\n", ret);
      return ret;
    }

    printk(KERN_INFO DEVICE_NAME " i2c slave registered %d\n", ret);
    device_registered = 1;
  }

  return 0;
}

void unregister_i2cslave(void) {
  if (device_registered) {
    printk(KERN_INFO DEVICE_NAME " unregistering i2c slave\n");
    platform_device_unregister(&i2cloopback_device);
    platform_driver_unregister(&i2cloopback_driver);
    device_registered = 0;
  }
}
