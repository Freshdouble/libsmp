using System;
using System.Collections.Generic;

namespace libsmp
{
    public class SMPMessageEventArgs : EventArgs
    {
        public List<byte> Data { get; set; }
    }
    public class SMP
    {
        private uint decoderstate = 0;
        private bool firstLengthbyteReceived = false;
        private uint currentcrc = 0;
        private List<byte> received = new List<byte>();
        private byte crcHighbyte = 0;
        private bool framestartReceivedLast = false;
        /***********************************************************************
        * @brief Private function definiton to calculate the crc checksum
        ***********************************************************************/
        protected static uint crc16(uint crc, uint c, uint mask)
        {
            byte i;
            uint _crc = crc;
            for (i = 0; i < 8; i++)
            {
                if (((_crc ^ c) & 1) == 1)
                {
                    _crc = (_crc >> 1) ^ mask;
                }
                else
                    _crc >>= 1;
                c >>= 1;
            }
            return (_crc);
        }

        protected void resetDecoderState()
        {
            decoderstate = 0;
            firstLengthbyteReceived = false;
            currentcrc = 0;
            received.Clear();
            crcHighbyte = 0;
            framestartReceivedLast = false;
        }

        /************************************************************************
         * @brief Initialize the smp-buffers
         ************************************************************************/
        public SMP()
        {
            resetDecoderState();
        }

        /**
         * @brief The number that is selected as the Framestart
         * */
        public byte Framestart { get; set; }

        /**
         * @brief The maximum accepted message length for transmit and receive
         * */
        public UInt16 MaxMessageLength { get; set; }

        /**
         * @brief The crc polynom to use
         * */
        public UInt16 CRCPolynom { get; set; }

        public event EventHandler MessageReceived;

        public event EventHandler RogueFrameReceived;

        /************************************************************************
         * @brief Estimate the number of bytes in the full smp packet.
         *
         * Since this function is faster than the calculation of the smp packet itself
         * one could check if the sendfunction is able to send all data and if it's not
         * avoid packet creation, especialy when using reed solomon codes
         *
         * @return estimated size of the smp packet
         ************************************************************************/

        public uint SMP_estimatePacketLength(byte[] buffer, uint length)
        {
            uint overheadCounter = 0;
            uint ret = 0;
            uint i;
            for (i = 0; i < length; i++)
            {
                if (buffer[i] == Framestart)
                    overheadCounter++;
            }
            ret = length + overheadCounter + 5;
            return ret;
        }

        /************************************************************************      
         * @brief Sends data to the smp outputbuffer
         * Create a smp packet from the data in buffer and writes it into messageBuffer
         * messageStartPtr points to the start of the smppacket
         * @return The length of the whole smp packet. If this value is zero an error
         *          occured and messageStartPtr is not valid
        /************************************************************************/
        public int SMP_Send(byte[] buffer, out List<byte> smpmessage)
        {
            uint i = 2;
            uint crc = 0;
            var payload = new List<byte>();
            if (buffer.Length > MaxMessageLength)
                throw new ArgumentException("The message length must not be bigger than the MaxMessageLength");

            for (i = 0; i < buffer.Length; i++)
            {
                if (buffer[i] == Framestart)
                {
                    payload.Add(Framestart);
                }

                payload.Add(buffer[i]);
                crc = crc16(crc, buffer[i], CRCPolynom);
            }

            byte crcbyte = (byte)((crc >> 8) & 0xFF);
            payload.Add(crcbyte); //CRC high byte

            if (crcbyte == Framestart)
            {
                payload.Add(Framestart);
            }
            crcbyte = (byte)(crc & 0xFF); //CRC low byte
            payload.Add(crcbyte);
            if (crcbyte == Framestart)
            {
                payload.Add(Framestart);
            }
            int packedLength = payload.Count;
            var header = new List<byte>();
            header.Add(Framestart);
            byte lengthbyte = (byte)(packedLength & 0xFF); //Lowbyte
            header.Add(lengthbyte);
            if (lengthbyte == Framestart)
            {
                header.Add(Framestart);
            }
            lengthbyte = (byte)((packedLength >> 8) & 0xFF); //Highbyte
            header.Add(lengthbyte);
            if (lengthbyte == Framestart)
            {
                header.Add(Framestart);
            }
            smpmessage = new List<byte>();
            smpmessage.AddRange(header);
            smpmessage.AddRange(payload);
            return smpmessage.Count;
        }

        /************************************************************************
         * @brief  Parse multiple bytes
         * This function calls SMP_RecieveInByte for every byte in the buffer
         ************************************************************************/
        public int SMP_RecieveInBytes(byte[] data)
        {
            uint i;
            int ret = 0;
            int smpRet;
            for (i = 0; i < data.Length; i++)
            {
                smpRet = SMP_RecieveInByte(data[i]);
                if (smpRet != 0)
                    ret = smpRet;
            }
            return ret;
        }

        /**
         *  @brief Private function to process the received bytes without the bytestuffing
         * **/
        public int private_SMP_RecieveInByte(byte data)
        {
            switch (decoderstate)
            //State machine
            {
                case 0: //Idle State Waiting for Framestart
                    BytesToReceive = 0;
                    firstLengthbyteReceived = false;
                    break;
                case 1:
                    Receiving = true;
                    if (!firstLengthbyteReceived)
                    {
                        BytesToReceive = data;
                        firstLengthbyteReceived = true;
                    }
                    else
                    {
                        BytesToReceive |= (uint)data << 8;
                        decoderstate = 2;
                        currentcrc = 0;
                    }
                    break;
                case 2:
                    BytesToReceive--;
                    received.Add(data);
                    currentcrc = crc16(currentcrc, data, CRCPolynom);
                    if (BytesToReceive == 2) //If we only have two bytes to receive we switch to the reception of the crc data
                    {
                        decoderstate = 3;
                    }
                    else if (BytesToReceive < 2)
                    {
                        //This should not happen and indicates an memorycorruption
                        decoderstate = 0;
                        BytesToReceive = 0;
                        Receiving = false;
                        return -3;
                    }
                    break;
                case 3:
                    if (BytesToReceive != 0)
                    {
                        crcHighbyte = data; //At first we save the high byte of the transmitted crc
                        BytesToReceive = 0; //Set bytestoReceive to 0 to signal the receiving of the last byte
                    }
                    else
                    {
                        int ret = -1;
                        if (currentcrc == ((uint)(crcHighbyte << 8) | data)) //Read the crc and compare
                        {
                            //Data ready
                            MessageReceived?.Invoke(this, new SMPMessageEventArgs()
                            {
                                Data = received
                            });
                            Receiving = false;
                            ret = 1;
                        }
                        else //crc doesnt match.
                        {
                            RogueFrameReceived?.Invoke(this, new SMPMessageEventArgs()
                            {
                                Data = received
                            });
                            Receiving = false;
                            ret = -1;
                        }
                        resetDecoderState();
                        return ret;
                    }
                    break;

                default: //Invalid State
                    resetDecoderState();
                    return -3;
            }
            return 0;
        }

        /************************************************************************
        * @brief Parser received byte
        * Call this function on every byte received from the interface
        ************************************************************************/
        public int SMP_RecieveInByte(byte data)
        {
            // Remove the bytestuffing from the data
            if (data == Framestart)
            {
                if (framestartReceivedLast)
                {
                    framestartReceivedLast = false;
                    return private_SMP_RecieveInByte(data);
                }
                else
                    framestartReceivedLast = true;
            }
            else
            {
                if (framestartReceivedLast)
                {
                    if (Receiving)
                    {
                        RogueFrameReceived?.Invoke(this, new SMPMessageEventArgs()
                        {
                            Data = received
                        });
                    }
                    resetDecoderState();
                    Receiving = true;
                    decoderstate = 1; //Set the decoder into receive mode
                }
                return private_SMP_RecieveInByte(data);
            }
            return 0;
        }

        /**********************************************************************
         * @brief Get the amounts of byte to recieve from the Interface for a full frame
         * This value is only valid if status.recieving is true
         * However if status.recieving is false this value should hold 0 but this
         * isn't confirmed yet
         **********************************************************************/
        public uint BytesToReceive { get; protected set; } = 0;

        /**
         * @brief Returns true if the smp stack for the smp_struct object is recieving
         */
        public bool Receiving { get; protected set; } = false;
    }
}
