using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using libRS;

namespace libSMP
{
    public class SMP
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

        public delegate int SMP_Frame_Ready(List<byte> fifo);
        public delegate ushort SMP_send_function(byte[] buffer, int length);

        public event SMP_Frame_Ready frameReceived;
        public event SMP_Frame_Ready rogueFrame;
        public event SMP_send_function send;

        public class Message
        {
            private byte[] _message;
            private uint _length;

            public Message(byte[] message, uint length)
            {
                _message = message;
                _length = length;
            }

            public Message(byte[] message) : this(message,(uint)message.Length)
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

        private Queue<Message> receivedMessages;
        private Queue<Message> messagesToSend;
          

        public SMP(bool useRS)
        {
            flags = new Flags_s();
            crc = 0;
            crcHighByte = 0;
            bytesToRecieve = 0;
            rsBuffer = new byte[Constants.Blocksize];
            rsPtr = 0;
            fifo = new List<byte>();

            messagesToSend = new Queue<Message>();
            receivedMessages = new Queue<Message>();
            if (useRS)
                rs = new ReedSolomon();
            else
                rs = null;
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
                if (send != null)
                    return (send(newBuffer, (int)newMessageSize + 1) == (newMessageSize + 1)) ? newMessageSize + 1 : 0;
                else
                {
                    Message msg = new Message(newBuffer);
                    messagesToSend.Enqueue(msg);
                    return newMessageSize + 1;
                }
            }
            else
            {
                if (send != null)
                    return (send(message2, (int)messageSize) == (messageSize)) ? messageSize : 0;
                else
                {
                    Message msg = new Message(message2);
                    messagesToSend.Enqueue(msg);
                    return messageSize;
                }
            }
            return 0;
        }

        public int RecieveInBytes(byte[] data, uint length)
        {
            uint i;
            int ret = 0;
            for (i = 0; i < length; i++)
            {
                ret = receiveByte(data[i]);
                if (ret < 0)
                    break;
            }
            return ret;
        }

        private int processByte(byte data)
        {
            switch (flags.status)
            //State machine
            {
                case 0: //Idle State Waiting for Framestart
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
                            if (frameReceived != null)
                                return frameReceived(fifo);
                            else
                            {
                                Message msg = new Message(fifo.ToArray());
                                receivedMessages.Enqueue(msg);
                                return 0;
                            }
                        }
                        else //crc doesnt match.
                        {
                            if (rogueFrame != null)
                                return rogueFrame(fifo);
                        }
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
                    break;
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
                        rogueFrame?.Invoke(fifo);
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

        public int receiveByte(byte data)
        {
            uint i;
            byte ret = 0;
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
                                ret = (byte)strippFramestart(rsBuffer[Framepos]);
                                uint cpyLength = Constants.Blocksize - Framepos - 1;
                                byte[] newBlock = new byte[cpyLength];
                                Array.Copy(newBlock, 0, rsBuffer, Framepos + 1, cpyLength);
                                Array.Copy(rsBuffer, 0, newBlock, 0, cpyLength);
                                rsPtr = cpyLength;
                                return ret;
                            }
                            return -1;
                        }
                    }
                    for (i = 0; i < Constants.BlockData; i++)
                    {
                        ret = (byte)strippFramestart(rsBuffer[i]);
                        if (ret < 0)
                            break;
                    }
                }
            }
            else
            {
                return strippFramestart(data);
            }
            return ret;
        }

        public int NumberMessagesToSend
        {
            get
            {
                return messagesToSend.Count;
            }
        }

        public int NumberMessagesToReceive
        {
            get
            {
                return receivedMessages.Count;
            }
        }

        public Message NextReceivedMessage
        {
            get
            {
                return receivedMessages.Dequeue();
            }
        }

        public Message NextSendMessage
        {
            get
            {
                return messagesToSend.Dequeue();
            }
        }
    }
}
