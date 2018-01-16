using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using libRS;

namespace libSMP
{
    public abstract class SMP : IPacketStream
    {
        private struct Flags_s
        {
            public bool recieving;
            public bool recievedDelimeter;
            public bool noCRC;
            public byte status;
        };

        private Flags_s flags;
        private uint crc;
        private byte crcHighByte;
        private uint bytesToRecieve;

        private ReedSolomon rs;
        private uint rsPtr;
        private byte[] rsBuffer;

        private List<byte> fifo;

        protected ITransmitionInterface inter;

        public event EventHandler MessageReceived;

        public SMP(bool useRS, ITransmitionInterface inter)
        {
            flags = new Flags_s();
            crc = 0;
            crcHighByte = 0;
            bytesToRecieve = 0;
            rsBuffer = new byte[Constants.Blocksize];
            rsPtr = 0;
            fifo = new List<byte>();

            inter.DataReceived += Inter_received;
            this.inter = inter;

            if (useRS)
                rs = new ReedSolomon();
            else
                rs = null;
        }

        private void Inter_received(object sender, Object e)
        {
            ITransmitionInterface inter = (ITransmitionInterface)sender;
            int bytesToRead = inter.BytesAvailable;
            byte[] buffer = new byte[bytesToRead];

            inter.Read(buffer, 0, bytesToRead);

            RecieveInBytes(buffer, (uint)buffer.Length);
        }

        /***********************************************************************
        * Private function definiton to calculate the crc checksum
        */
        private uint crc16(uint crc, uint c, uint mask)
        {
            byte i;
            uint _crc = crc;
            for (i = 0; i < 8; i++)
            {
                if (((_crc ^ c) & 1) != 0)
                {
                    _crc = (_crc >> 1) ^ mask;
                }
                else
                    _crc >>= 1;
                c >>= 1;
            }
            return (_crc);
        }

        /************************************************************************
        * @brief Estimate the number of bytes in the full smp packet.
        *
        * Since this function is faster than the calculation of the smp packet itself
        * one could check if the sendfunction is able to send all data and if it's not
        * avoid packet creation, especialy when using reed solomon codes
        *
        * @return estimated size of the smp packet
        ************************************************************************/

        public uint estimatePacketLength(byte[] buffer, uint length)
        {
            ushort overheadCounter = 0;
            uint ret = 0;
            for (ushort i = 0; i < length; i++)
            {
                if (buffer[i] == Constants.FRAMESTART)
                    overheadCounter++;
            }
            ret = length + overheadCounter + 5;
            if (rs != null)
            {
                byte Blocknumber = (byte)(ret / Constants.BlockData);
                ret = ret * Constants.Blocksize;
                if (ret / (double)Constants.BlockData > Blocknumber)
                    ret += Constants.Blocksize;
            }
            return ret;
        }

        /************************************************************************/
        /* Sends data to the smp outputbuffer                                   */

        /************************************************************************/

        public uint SendData(byte[] buffer, uint length)
        {
            uint i = 2;
            uint offset = 0;
            uint crc = 0;

            if (length > Constants.MAX_PAYLOAD)
                return 0;

            byte[] message1 = new byte[2 * (length + 2)];
            byte[] message2 = new byte[2 * (length + 2) + 5];

            for (i = 0; i < length; i++)
            {

                if (buffer[i] == Constants.FRAMESTART)
                {
                    message1[i + offset] = (byte)Constants.FRAMESTART;
                    offset++;
                }

                message1[i + offset] = buffer[i];
                crc = crc16(crc, buffer[i], Constants.CRC_POLYNOM);
            }

            message1[i + offset] = (byte)(crc >> 8); //CRC high byte
            if (message1[i + offset] == Constants.FRAMESTART)
            {
                offset++;
                message1[i + offset] = (byte)Constants.FRAMESTART;
            }
            message1[i + offset + 1] = (byte)(crc & 0xFF); //CRC low byte
            if (message1[i + offset + 1] == Constants.FRAMESTART)
            {
                offset++;
                message1[i + offset + 1] = (byte)Constants.FRAMESTART;
            }

            uint packageSize = length + 2;
            uint completeFramesize = packageSize + offset;
            offset = 0;
            message2[0] = (byte)Constants.FRAMESTART;
            message2[1] = (byte)(packageSize & 0xFF);
            if (message2[1] == Constants.FRAMESTART)
            {
                message2[2] = (byte)Constants.FRAMESTART;
                offset = 1;
            }
            message2[2 + offset] = (byte)(packageSize >> 8);
            if (message2[2 + offset] == Constants.FRAMESTART)
            {
                message2[3 + offset] = (byte)Constants.FRAMESTART;
                offset = 2;
            }

            for (i = 3 + offset; i < completeFramesize + 3 + offset; i++)
            {
                message2[i] = message1[i - (3 + offset)];
            }

            uint messageSize = completeFramesize + 3 + offset;

            if (rs != null)
            {
                messageSize--;
                uint BlockNumber = messageSize / Constants.BlockData;
                if (BlockNumber * Constants.BlockData < messageSize)
                {
                    BlockNumber++;
                }
                uint newMessageSize = BlockNumber * Constants.Blocksize;
                byte[] newBuffer = new byte[newMessageSize + 1];
                byte[] CurrentBlock = new byte[Constants.BlockData];
                uint ptr = 1;
                uint newPtr = 1;
                for (i = 0; i < BlockNumber; i++)
                {
                    if ((messageSize - ptr + 1) >= Constants.BlockData)
                    {
                        Array.Copy(message2, ptr, CurrentBlock, 0, Constants.BlockData);
                        ptr += Constants.BlockData;
                    }
                    else
                    {
                        Array.Clear(CurrentBlock, 0, (int)Constants.BlockData);
                        Array.Copy(message2, ptr, CurrentBlock, 0, messageSize - ptr + 1);
                        ptr = messageSize;
                    }
                    byte[] encodedData = new byte[Constants.Blocksize];
                    rs.encode_data(CurrentBlock, (int)Constants.BlockData, ref encodedData);
                    Array.Copy(encodedData, 0, newBuffer, newPtr, (int)Constants.Blocksize);
                    newPtr += Constants.Blocksize;
                    if (ptr == messageSize)
                        break;
                }
                newBuffer[0] = (byte)Constants.FRAMESTART;
                inter.Write(newBuffer, 0, (int)newMessageSize + 1);
                return length;
            }
            else
            {
                inter.Write(message2, 0, (int)messageSize);
                return length;
            }
        }

        public int RecieveInBytes(byte[] data, uint length)
        {
            uint i;
            int ret = 0;
            int smpRet;
            for (i = 0; i < length; i++)
            {
                smpRet = receiveByte(data[i]);
                if (smpRet != 0)
                    ret = smpRet;
            }
            return ret;
        }

        private int processByte(byte data)
        {
            switch (flags.status)
            //State machine
            {
                case 0: //Idle State Waiting for Framestart
                    rogueFrameReceived(new List<byte>() { data });
                    break;
                case 1:
                    if (bytesToRecieve == 0)
                    {
                        bytesToRecieve = data;
                    }
                    else
                    {
                        bytesToRecieve |= (uint)(data << 8);
                        flags.status = 2;
                        fifo.Clear();
                        crc = 0;
                    }
                    break;
                case 2:
                    if (bytesToRecieve == 2) //Only CRC to recieve
                    {
                        flags.status = 3;
                    }
                    else
                    {
                        bytesToRecieve--;
                        fifo.Add(data);
                        /****************************************
                        if (!fifo_write_byte(data, st->buffer))
                        {
                            //Bufferoverflow
                            flags.status = 0;
                            flags.recieving = false;
                            rsPtr = 0;
                            return -1;
                        }
                        ****************************************/
                        crc = crc16(crc, data, Constants.CRC_POLYNOM);
                        break;
                    }
                    goto case 3;
                case 3:
                    if (bytesToRecieve == 0)
                    {
                        flags.status = 0;
                        flags.recieving = false;
                        rsPtr = 0;
                        if (crc == ((crcHighByte << 8) | data)) //Read the crc and compare
                        {
                            //Data ready
                            frameReceived(fifo);
                            MessageReceived?.Invoke(this, null);
                        }
                        else //crc doesnt match.
                        {
                            rogueFrameReceived(fifo);
                        }
                        fifo.Clear();
                    }
                    else
                    {
                        crcHighByte = data; //Save first byte of crc
                        bytesToRecieve = 0;
                    }
                    break;

                default: //Invalid State
                    flags.status = 0;
                    bytesToRecieve = 0;
                    flags.recieving = false;
                    rsPtr = 0;
                    return -3;
            }
            return 0;
        }

        private int strippFramestart(byte data)
        {
            if (data == Constants.FRAMESTART)
            {
                if (flags.recievedDelimeter)
                {
                    flags.recievedDelimeter = false;
                    return processByte(data);
                }
                else
                    flags.recievedDelimeter = true;
            }
            else
            {
                if (flags.recievedDelimeter)
                {
                    flags.status = 1;
                    bytesToRecieve = 0;
                    flags.recievedDelimeter = false;
                    if (flags.recieving)
                    {
                        rogueFrameReceived(fifo);
                    }
                    flags.recieving = true;
                }
                return processByte(data);
            }
            return 0;
        }

        private uint seekFramestart(byte[] data, uint length)
        {
            uint i;
            for (i = 0; i < length; i++)
            {
                if (data[i] == Constants.FRAMESTART)
                    return i + 1;
            }
            return 0;
        }

        protected int receiveByte(byte data)
        {
            uint i;
            byte ret = 0;
            byte smpRet = 0;
            if ((flags.recieving || flags.recievedDelimeter) && rs != null)
            {
                rsBuffer[rsPtr] = data;
                rsPtr++;
                if (rsPtr >= Constants.Blocksize)
                {
                    rsPtr = 0;
                    rs.decode_data(rsBuffer, (int)Constants.Blocksize);
                    if (rs.check_syndrome() != 0)
                    {
                        if (rs.correct_errors_erasures(ref rsBuffer, (int)Constants.Blocksize, 0, null) == 0)
                        {
                            flags.status = 0;
                            bytesToRecieve = 0;
                            flags.recieving = false;
                            uint Framepos = seekFramestart(rsBuffer, Constants.Blocksize);
                            if (Framepos > 0)
                            {
                                Framepos--;
                                strippFramestart(rsBuffer[Framepos]);
                                uint cpyLength = Constants.Blocksize - Framepos - 1;
                                byte[] newBlock = new byte[cpyLength];
                                Array.Copy(newBlock, 0, rsBuffer, Framepos + 1, cpyLength);
                                Array.Copy(rsBuffer, 0, newBlock, 0, cpyLength);
                                rsPtr = cpyLength;
                                return -4;
                            }
                            return -5;
                        }
                    }
                    for (i = 0; i < Constants.BlockData; i++)
                    {
                        smpRet = (byte)strippFramestart(rsBuffer[i]);
                        if (smpRet != 0)
                            ret = smpRet;
                    }
                }
            }
            else
            {
                return strippFramestart(data);
            }
            return ret;
        }

        public bool CanRead => false;

        public bool CanSeek => false;

        public bool CanWrite => inter.CanWrite;

        public abstract int Length { get; }

        public void Flush()
        {
            inter.Flush();
        }

        public void Write(Message m)
        {
            SendData(m.message, m.length);
        }

        protected abstract void frameReceived(List<byte> data);
        protected abstract void rogueFrameReceived(List<byte> data);

        public abstract Message Read();
    }
}
