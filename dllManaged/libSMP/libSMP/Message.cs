using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class Message : ReadOnlyCollection<byte>
    {
        public Message(IList<byte> list) : base(list)
        {
        }

        public byte[] message
        {
            get
            {
                return this.ToArray();
            }
        }

        public uint length
        {
            get
            {
                return (uint)Count;
            }
        }

        public override string ToString()
        {
            return Encoding.UTF8.GetString(this.ToArray());
        }
    }
}
