using System;
using System.Collections.Generic;

namespace libsmp
{
    public class ManagedSMP : SMP
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

        protected void resetDecoderState(bool preserveReceivedDelimeter)
        {
            decoderstate = 0;
            firstLengthbyteReceived = false;
            currentcrc = 0;
            received.Clear();
            crcHighbyte = 0;
            if(!preserveReceivedDelimeter)
            {
                framestartReceivedLast = false;
            }
        }

        public ManagedSMP()
        {
            resetDecoderState(false);
        }

        /**
         * @brief The number that is selected as the Framestart
         * */
        public byte Framestart { get; set; } = 0xFF;

        /**
         * @brief The maximum accepted message length for transmit and receive
         * */
        public ushort MaxMessageLength { get; set; } = 65500;

        /**
         * @brief The crc polynom to use
         * */
        public ushort CRCPolynom { get; set; } = 0xA001;

        /************************************************************************      
         * @brief Generates a full SMP paket with the specified payload
        /************************************************************************/
        public override byte[] GenerateMessage(byte[] payload)
        {
            uint i = 2;
            uint crc = 0;
            var payloadBuffer = new List<byte>();
            if (payload.Length > MaxMessageLength)
                throw new ArgumentException("The message length must not be bigger than the MaxMessageLength");

            for (i = 0; i < payload.Length; i++)
            {
                if (payload[i] == Framestart)
                {
                    payloadBuffer.Add(Framestart);
                }

                payloadBuffer.Add(payload[i]);
                crc = crc16(crc, payload[i], CRCPolynom);
            }

            byte crcbyte = (byte)((crc >> 8) & 0xFF);
            payloadBuffer.Add(crcbyte); //CRC high byte

            if (crcbyte == Framestart)
            {
                payloadBuffer.Add(Framestart);
            }
            crcbyte = (byte)(crc & 0xFF); //CRC low byte
            payloadBuffer.Add(crcbyte);
            if (crcbyte == Framestart)
            {
                payloadBuffer.Add(Framestart);
            }
            int packedLength = payload.Length + 2;
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
            var smpmessage = new byte[header.Count + payloadBuffer.Count];
            header.CopyTo(smpmessage);
            payloadBuffer.CopyTo(smpmessage, header.Count);
            return smpmessage;            
        }

        /************************************************************************
         * @brief  Parse multiple bytes
         * This function calls SMP_RecieveInByte for every byte in the buffer
         ************************************************************************/
        public override sbyte ProcessBytes(byte[] data)
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
            return (sbyte)ret;
        }

        /**
         *  @brief Private function to process the received bytes without the bytestuffing
         * **/
        private int private_SMP_RecieveInByte(byte data)
        {
            switch (decoderstate)
            //State machine
            {
                case 0: //Idle State Waiting for Framestart
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
                        received.Clear();
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
                        resetDecoderState(true);
                        return -2;
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
                        int ret = -4;
                        if (currentcrc == ((uint)(crcHighbyte << 8) | data)) //Read the crc and compare
                        {
                            //Data ready
                            receivedmessages.Enqueue(received.ToArray());
                            Receiving = false;
                            ret = 1;
                            resetDecoderState(false);
                        }
                        else //crc doesnt match.
                        {
                            /**
                            RogueFrameReceived?.Invoke(this, new SMPMessageEventArgs()
                            {
                                Data = received
                            });**/
                            Receiving = false;
                            ret = -1;
                            resetDecoderState(true);
                        }
                        return ret;
                    }
                    break;

                default: //Invalid State
                    resetDecoderState(true);
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
                        /**
                        RogueFrameReceived?.Invoke(this, new SMPMessageEventArgs()
                        {
                            Data = received
                        });
                        **/
                    }
                    resetDecoderState(false);
                    Receiving = true;
                    decoderstate = 1; //Set the decoder into receive mode
                }
                framestartReceivedLast = false;
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