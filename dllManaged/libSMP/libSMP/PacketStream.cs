using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace libSMP
{
    public abstract class  PacketStream
    {
        public abstract bool CanRead { get; }

        public abstract bool CanSeek { get; }

        public abstract bool CanWrite { get; }

        public abstract int Length { get; }

        public abstract void Flush();

        public abstract Message Read();

        public abstract void Write(Message m);

        public event EventHandler MessageReceived;

        protected void FireMessageReceived()
        {
            MessageReceived?.Invoke(this, null);
        }
    }
}
