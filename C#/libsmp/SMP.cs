using System;
using System.Collections.Generic;

namespace libsmp
{
    public abstract class SMP : ISMP
    {
        protected Queue<byte[]> receivedmessages = new Queue<byte[]>();
        public int StoredMessages => receivedmessages.Count;

        public bool IsDisposed {get; protected set;} = false;

        public virtual void Dispose()
        {
            IsDisposed = true;
        }
        public abstract byte[] GenerateMessage(byte[] payload);
        public virtual byte[] GetMessage()
        {
            return receivedmessages.Dequeue();
        }
        public abstract sbyte ProcessBytes(byte[] data);
    }
}