using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class SMPPolling : SMPOutputBuffer
    {
        protected class Interface : ITransmitionInterface
        {
            private ChunkQueu<byte> bytesToSend;
            private ChunkQueu<byte> bytesReceived;

            public event DataReceivedEvent DataReceived;

            public bool CanRead => BytesAvailable > 0;

            public bool CanWrite => true;

            public int BytesAvailable => bytesReceived.Count;

            public int BytesToSendCount => bytesToSend.Count;

            public Interface()
            {
                bytesToSend = new ChunkQueu<byte>();
                bytesReceived = new ChunkQueu<byte>();
            }

            public void Flush()
            {
                
            }

            public int Read(byte[] buffer, int offset, int count)
            {
                byte[] data = bytesReceived.DequeueChunk(count).ToArray();
                Array.Copy(data, 0, buffer, offset, data.Length);
                return data.Length;
            }

            public void Write(byte[] buffer, int offset, int count)
            {
                byte[] data = new byte[count];
                for (int i = offset; i < (offset + count); i++)
                {
                    data[i - offset] = buffer[i];
                }

                bytesToSend.EnqueuChunk(data);
            }

            public void addReceivedData(IEnumerable<byte> data)
            {
                bytesReceived.EnqueuChunk(data);
                DataReceived?.Invoke(this, null);
            }

            public IEnumerable<byte> getDataToSend(int count)
            {
                return bytesToSend.DequeueChunk(count);
            }
        }

        public SMPPolling(bool useRS) : base(useRS, new Interface())
        {
        }

        public IEnumerable<byte> getDataToSend(int count)
        {
            return ((Interface)inter).getDataToSend(count);
        }

        public void addReceivedData(IEnumerable<byte> data)
        {
            ((Interface)inter).addReceivedData(data);
        }

        public int DataToSendCount => ((Interface)inter).BytesToSendCount;
    }
}
