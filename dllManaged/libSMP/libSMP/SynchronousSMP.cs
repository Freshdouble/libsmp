using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace libSMP
{
    public class SynchronousSMP : SMP
    {
        private SynchronousSMP(bool useRS, ITransmitionInterface inter) : base(useRS, inter)
        {
        }

        public SynchronousSMP(bool useRS)
        {
            this(useRS, this);
        }
    }
}
