using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public abstract class TransmitionInterface : ITransmitionInterface
    {
        public abstract bool CanRead { get; }
        public abstract bool CanWrite { get; }

        public abstract void Flush();
        public abstract int Read(byte[] buffer, int offset, int count);
        public abstract int Write(byte[] buffer, int offset, int count);

        public event EventHandler receive;

        protected void raiseReceiveEvent()
        {
            receive?.Invoke(this, null);
        }
    }
}
