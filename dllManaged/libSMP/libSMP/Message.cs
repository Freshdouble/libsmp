using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class Message
    {
        private byte[] _message;
        private uint _length;

        public Message(byte[] message, uint length)
        {
            _message = new byte[length];
            Array.Copy(message, 0, _message, 0, length);
            _length = length;
        }

        public Message(byte[] message) : this(message, (uint)message.Length)
        {

        }

        public byte[] message
        {
            get
            {
                return _message;
            }
        }

        public uint length
        {
            get
            {
                return _length;
            }
        }
    }
}
