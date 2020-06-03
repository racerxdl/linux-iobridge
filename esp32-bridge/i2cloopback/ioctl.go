package i2cloopback

import (
	"errors"
	"fmt"
	"io"
	"syscall"
)

// Defines for struct i2c_msg - flags field
const (
	I2C_M_RD           = 0x0001 /* read data, from slave to master */
	I2C_M_TEN          = 0x0010 /* this is a ten bit chip address */
	I2C_M_RECV_LEN     = 0x0400 /* length will be first received byte */
	I2C_M_NO_RD_ACK    = 0x0800 /* if I2C_FUNC_PROTOCOL_MANGLING */
	I2C_M_IGNORE_NAK   = 0x1000 /* if I2C_FUNC_PROTOCOL_MANGLING */
	I2C_M_REV_DIR_ADDR = 0x2000 /* if I2C_FUNC_PROTOCOL_MANGLING */
	I2C_M_NOSTART      = 0x4000 /* if I2C_FUNC_NOSTART */
	I2C_M_STOP         = 0x8000 /* if I2C_FUNC_PROTOCOL_MANGLING */
)

const (
	IOCTL_GET_I2C_MSG = 0x1000
	IOCTL_COMMIT_MSG  = 0x2000
)

const (
	IOCTL_RET_SUCCESS = 0
	IOCTL_RET_NO_SRV  = 1
	IOCTL_RET_NO_MSG  = 2
	IOCTL_RET_FAIL    = 3
)

var (
	ErrorNoMessage           = errors.New("no message")
	ErrorReadRequestMessage  = errors.New("read request message")
	ErrorWriteRequestMessage = errors.New("write request message")
)

type LoopbackMessage struct {
	addr  uint16
	flags uint16
	len   uint16
}

func (m LoopbackMessage) String() string {
	return fmt.Sprintf("{ addr: %04x, flags: %04x, len: %d }", m.addr, m.flags, m.len)
}

func ioctl(fd, cmd, arg uintptr) (int, error) {
	var err error
	r1, _, errno := syscall.Syscall6(syscall.SYS_IOCTL, fd, cmd, arg, 0, 0, 0)
	if errno != syscall.Errno(0) {
		err = fmt.Errorf("error %d", errno)
	}
	return int(r1), err
}

type I2CMessage interface {
	io.Writer
	io.Reader

	Commit() error
	GetAddr() uint16
	GetFlags() uint16
	GetLen() uint16
	IsReadRequest() bool
}
