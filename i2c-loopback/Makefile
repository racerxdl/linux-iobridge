obj-m += i2cloopback.o
i2cloopback-objs += i2cmaster.o i2cslave.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean