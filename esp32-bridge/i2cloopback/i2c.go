package i2cloopback

import (
	"fmt"
	"github.com/quan-to/slog"
	"os"
	"time"
	"unsafe"
)

var log = slog.Scope("I2CC")

type I2CController struct {
	rc      *os.File
	running bool
	i2cmsgs chan I2CMessage
	commit  chan bool
}

//
func NewI2C(bus int) (*I2CController, error) {
	f, err := os.OpenFile(fmt.Sprintf("/dev/i2c-%d-master", bus), os.O_RDWR, 0600)
	if err != nil {
		return nil, err
	}

	v := &I2CController{
		rc:      f,
		i2cmsgs: make(chan I2CMessage),
		commit:  make(chan bool),
	}

	return v, nil
}

func (c *I2CController) Start() {
	if !c.running {
		c.running = true
		go c.loop()
	}
}

func (c *I2CController) Stop() {
	if c.running {
		c.running = true
	}
}

func (c *I2CController) GetChannel() chan I2CMessage {
	return c.i2cmsgs
}

func (c *I2CController) GetMessage() (LoopbackMessage, error) {
	msg := LoopbackMessage{}
	ret, err := ioctl(c.rc.Fd(), IOCTL_GET_I2C_MSG, uintptr(unsafe.Pointer(&msg)))
	if err != nil {
		return msg, err
	}

	if ret == IOCTL_RET_NO_MSG {
		return msg, ErrorNoMessage
	}

	return msg, nil
}

func (c *I2CController) loop() {
	log.Debug("I2C Controller Loop Started")
	for c.running {
		msg, err := c.GetMessage()
		if err != nil && err != ErrorNoMessage {
			log.Error("Error receiving message: %s", err)
			break
		}

		if err == ErrorNoMessage {
			time.Sleep(time.Millisecond)
			continue
		}

		// Send message to channel
		c.i2cmsgs <- MakeI2CMessage(c, msg)

		// Wait for commit
		<-c.commit
	}
	log.Debug("I2C Controller Loop Stopped")
}

func (c *I2CController) read(data []byte) (n int, err error) {
	return c.rc.Read(data)
}

func (c *I2CController) write(data []byte) (n int, err error) {
	return c.rc.Write(data)
}

func (c *I2CController) Commit() error {
	log.Debug("Commiting message")
	_, err := ioctl(c.rc.Fd(), IOCTL_COMMIT_MSG, 0)
	c.commit <- true
	return err
}
