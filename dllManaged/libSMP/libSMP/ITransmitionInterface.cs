using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public delegate void DataReceivedEvent(Object sender, Object eventArgs);
    public interface ITransmitionInterface
    {
        void Write(byte[] buffer, int offset, int count);
        int Read(byte[] buffer, int offset, int count);
        void Flush();
        bool CanRead { get; }
        bool CanWrite { get; }

        int BytesAvailable { get; }

        event DataReceivedEvent DataReceived;
    }
}
