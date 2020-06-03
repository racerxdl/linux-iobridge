package i2cloopback

type i2cMessage struct {
	LoopbackMessage
	c *I2CController
}

func (m i2cMessage) GetAddr() uint16 {
	return m.addr
}

func (m i2cMessage) GetFlags() uint16 {
	return m.flags
}

func (m i2cMessage) GetLen() uint16 {
	return m.len
}

func (m i2cMessage) IsReadRequest() bool {
	return (m.flags & I2C_M_RD) > 0
}

func (m i2cMessage) Write(p []byte) (n int, err error) {
	if !m.IsReadRequest() {
		return -1, ErrorWriteRequestMessage
	}

	return m.c.write(p)
}

func (m i2cMessage) Read(p []byte) (n int, err error) {
	if m.IsReadRequest() {
		return -1, ErrorReadRequestMessage
	}

	return m.c.read(p)
}

func (m i2cMessage) Commit() error {
	return m.c.Commit()
}

func MakeI2CMessage(c *I2CController, message LoopbackMessage) I2CMessage {
	i := &i2cMessage{
		c:               c,
		LoopbackMessage: message,
	}
	return i
}
