using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public interface ITransmitionInterface
    {
        int Write(byte[] buffer, int offset, int count);
        int Read(byte[] buffer, int offset, int count);
        void Flush();
        bool CanRead { get; }
        bool CanWrite { get; }
    }
}
