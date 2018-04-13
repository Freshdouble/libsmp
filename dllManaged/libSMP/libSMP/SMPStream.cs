using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

namespace libSMP
{
    public class SMPStream : SMPOutputBuffer
    {
        private class StreamInterface : ITransmitionInterface
        {
            private Stream stream;
            private Queue<byte> data;
            private Thread thread;
            private bool run;
            private Mutex mut;

            public StreamInterface(Stream stream)
            {
                mut = new Mutex();
                run = true;
                stream = Stream.Synchronized(stream);
                stream.ReadTimeout = 500;
                this.stream = stream;
                data = new Queue<byte>();

                thread = new Thread(PollingThread);
                thread.Start();
            }

            ~StreamInterface()
            {
                run = false;
                try
                {
                    thread.Join(3000);
                }
                catch (Exception)
                {

                }
            }

            public bool CanRead => data.Count > 0;

            public bool CanWrite => true;

            public int BytesAvailable => data.Count;

            public event DataReceivedEvent DataReceived;

            public void Flush()
            {
                stream.Flush();
            }

            public int Read(byte[] buffer, int offset, int count)
            {
                int i = offset;
                mut.WaitOne();
                try
                {
                    for (; i < (offset + count); i++)
                    {
                        if (data.Count == 0)
                            break;
                        buffer[i] = data.Dequeue();
                    }
                }
                finally
                {
                    mut.ReleaseMutex();
                }
                return i - offset;
            }

            public void Write(byte[] buffer, int offset, int count)
            {
                stream.Write(buffer, offset, count);
                Flush();
            }

            private void PollingThread()
            {
                byte[] data = new byte[200];
                while(run)
                {
                    try
                    {
                        int readedBytes = stream.Read(data, 0, data.Length);
                        if (readedBytes > 0)
                        {
                            mut.WaitOne();
                            try
                            {
                                for (int i = 0; i < readedBytes; i++)
                                {
                                    this.data.Enqueue(data[i]);
                                }
                            }
                            finally
                            {
                                mut.ReleaseMutex();
                            }
                            DataReceived?.Invoke(this, null);
                        }
                    }
                    catch(Exception)
                    {

                    }
                    finally
                    {
                        Thread.Sleep(200);
                    }
                }
            }
        }
        public SMPStream(bool useRS, Stream stream) : base(useRS, new StreamInterface(stream))
        {
        }
    }
}
