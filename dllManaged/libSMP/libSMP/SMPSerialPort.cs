using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;

namespace libSMP
{
    class SMPSerialPort : SMPOutputBuffer
    {
        public class SerialPortInterface : SerialPort, ITransmitionInterface
        {
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
