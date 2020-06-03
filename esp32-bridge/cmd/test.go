package main

import (
	"github.com/quan-to/slog"
	"github.com/racerxdl/linux-iobridge/esp32-bridge/i2cloopback"
	"io"
)

var log = slog.Scope("MyHueController")

var dataBuffer []uint8
var currentAddress uint16

func WriteData(w io.Writer, offset, length int) {
	if offset+length > len(dataBuffer) {
		log.Error("Error: trying to send data beyond buffer")
		return
	}
	n, err := w.Write(dataBuffer[offset : offset+length])
	if err != nil {
		log.Error("Error writing bytes: %s", err)
	}
	if n != length {
		log.Error("Expected to write %d bytes but got %d", length, n)
	}
}

func ProcessMessage(msg i2cloopback.I2CMessage) {
	if msg.IsReadRequest() {
		// READ Request
		readBytes := uint16(len(dataBuffer)) - msg.GetLen()
		if readBytes > msg.GetLen() {
			readBytes = msg.GetLen()
		}
		WriteData(msg, int(currentAddress), int(readBytes))
	} else {
		// WRITE Request
		l := int(msg.GetLen())
		if l > 0 {
			buff := make([]uint8, l)
			n, err := msg.Read(buff)
			if n != l {
				log.Error("Expected %d bytes got %d", l, n)
			} else if err == nil {
				currentAddress = uint16(buff[0])
			} else {
				log.Error("Error reading buffer: %s", err)
			}
		}
	}

	err := msg.Commit()
	if err != nil {
		log.Error("Error committing message: %s", err)
	}
}

func main() {
	con, err := i2cloopback.NewI2C(0)
	if err != nil {
		panic(err)
	}

	currentAddress = 0
	dataBuffer = make([]uint8, 256)

	copy(dataBuffer, "HUEBR WORLD --- Now this fucking stuff is working\x00\x01\x02\x03\x04\x05")

	ch := con.GetChannel()

	con.Start()

	for {
		select {
		case msg := <-ch:
			//log.Debug("Received message")
			ProcessMessage(msg)
		}
	}

	con.Stop()
}
