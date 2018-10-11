using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.ComponentModel;

namespace libSMP
{
    public class SMPSerialPort : SMPOutputBuffer
    {
        public class SerialPortInterface : SerialPort, ITransmitionInterface
        {
            public SerialPortInterface()
            {
            }

            public SerialPortInterface(string portName) : base(portName)
            {
            }

            public SerialPortInterface(IContainer container) : base(container)
            {
            }

            public SerialPortInterface(string portName, int baudRate) : base(portName, baudRate)
            {
            }

            public SerialPortInterface(string portName, int baudRate, Parity parity) : base(portName, baudRate, parity)
            {
            }

            public SerialPortInterface(string portName, int baudRate, Parity parity, int dataBits) : base(portName, baudRate, parity, dataBits)
            {
            }

            public SerialPortInterface(string portName, int baudRate, Parity parity, int dataBits, StopBits stopBits) : base(portName, baudRate, parity, dataBits, stopBits)
            {
            }

            public bool CanRead => BytesAvailable > 0;

            public bool CanWrite => BaseStream.CanWrite;

            public int BytesAvailable => BytesToRead;

            event DataReceivedEvent ITransmitionInterface.DataReceived
            {
                add
                {
                    DataReceived += new SerialDataReceivedEventHandler(value);
                }

                remove
                {
                    DataReceived -= new SerialDataReceivedEventHandler(value);
                }
            }

            public void Flush()
            {
                BaseStream.Flush();
            }
        }
        public SMPSerialPort(bool useRS, SerialPortInterface port) : base(useRS, port)
        {
            if (!port.IsOpen)
                port.Open();
        }
    }
}
