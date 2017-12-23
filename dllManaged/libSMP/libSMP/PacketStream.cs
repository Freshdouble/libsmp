using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace libSMP
{
    public interface  IPacketStream
    {
        bool CanRead { get; }

        bool CanSeek { get; }

        bool CanWrite { get; }

        int Length { get; }

        void Flush();

        Message Read();

        void Write(Message m);

        event EventHandler MessageReceived;
    }
}
