#include <linux/i2c.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>

#define IOCTL_RET_SUCCESS  0
#define IOCTL_RET_NO_SRV   1
#define IOCTL_RET_NO_MSG   2
#define IOCTL_RET_FAIL     3

struct loopback_msg {
  __u16 addr;   /* slave address */
  __u16 flags;  /* see above for flag definitions */
  __u16 len;    /* msg length */
};

#define IOCTL_GET_I2C_MSG  0x1000
#define IOCTL_COMMIT_MSG   0x2000

#define BUFFER_SIZE 256

char data[BUFFER_SIZE];

char *myString = "HUEBR WORLD!!!\x00";
char buff[16];

int main() {
  struct loopback_msg myMsg;
  int fd;
  char *buff;

  printf("IOCTL_GET_I2C_MSG: %08x\n", IOCTL_GET_I2C_MSG);
  printf("IOCTL_COMMIT_MSG: %08x\n", IOCTL_COMMIT_MSG);
  printf("Sizeof: %04d\n", sizeof(struct loopback_msg));

  if ((fd = open("/dev/i2c-0-master", O_RDWR))<0) {
    printf("No such device /dev/i2c-0-master\n");
    exit(0);
  }

  int waiting = 1;
  int n;
  buff[0] = 0;

  for (int i = 0; i < 256; i++) {
    data[i] = 0x00;
  }

  memcpy(data, myString, strlen(myString));

  while (waiting) {
    int result = ioctl(fd, IOCTL_GET_I2C_MSG, &myMsg);
    switch (result) {
      case IOCTL_RET_NO_MSG:
        // printf("No message\n");
        break;
      case IOCTL_RET_NO_SRV:
        printf("No service available\n");
        waiting = 0;
        break;
      case IOCTL_RET_FAIL:
        printf("Fail to receive buffer\n");
        waiting = 0;
        break;
      case IOCTL_RET_SUCCESS:
        // printf("Received message!");
        // printf("Addr %02x Length: %d Flags: %d\n", myMsg.addr, myMsg.len, myMsg.flags);

        if (myMsg.flags & I2C_M_RD) { // READ
          uint8_t count = buff[0];
          printf("Reading from addr %02x\n", count);
          int haveBytes = BUFFER_SIZE - count;
          if (haveBytes > myMsg.len) {
            haveBytes = myMsg.len;
          }
          n = write(fd, data + count, haveBytes);
          if (n != haveBytes) {
            printf("Error writing content. Expected %ld got %d\n",haveBytes, n);
          }
        } else { // WRITE
          read(fd, buff, myMsg.len);
          printf("Address set to %02x\n", (uint8_t)buff[0]);
        }

        // printf("Commiting\n");
        n = ioctl(fd, IOCTL_COMMIT_MSG, &myMsg);
        // printf("Commited %d\n", n);
        // waiting = 0;
        break;
    }
    usleep(1);
  }

  close(fd);

  printf("Done\n");

  return 0;
}
