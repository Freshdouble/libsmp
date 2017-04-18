using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace libSMP
{
    static class Constants
    {
        public static uint FRAMESTART = 0xFF;
        public static uint CRC_POLYNOM = 0xA001;
        public static uint Blocksize = 16;
        public static uint BlockData = Blocksize - (uint)libRS.Constants.NPAR;
        public static uint MAX_PAYLOAD = 65000;
    }
}
