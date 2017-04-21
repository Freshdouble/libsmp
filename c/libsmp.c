/*****************************************************************************************************
 File: libsmp
 Autor: Peter Kremsner
 Date: 12.9.2014

 Basic data transmission protocoll to ensure data integrity

 ******************************************************************************************************/
#include "libsmp.h"
#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PAYLOAD 65534

#define USE_RS_CODE

/***********************************************************************
 * Private function definiton to calculate the crc checksum
 */
static unsigned int crc16(unsigned int crc, unsigned int c, unsigned int mask)
{
    unsigned char i;
    unsigned int _crc = crc;
    for (i = 0; i < 8; i++)
    {
        if ((_crc ^ c) & 1)
        {
            _crc = (_crc >> 1) ^ mask;
        }
        else
            _crc >>= 1;
        c >>= 1;
    }
    return (_crc);
}

static int seekFramestart(const byte* data, unsigned int length)
{
    unsigned int i;
    for (i = 0; i < length; i++)
    {
        if (data[i] == FRAMESTART)
            return i;
    }
    return -1;
}
#ifdef USE_RS_CODE
static unsigned short SMP_internal_EncodeRS(const byte* buffer, unsigned short length, smp_struct_t *st)
{
    if(length == 0)
        return 0;
    length--;
    unsigned int BlockNumber = length / BlockData;
    if (BlockNumber * BlockData < length)
    {
        BlockNumber++;
    }
    unsigned char newBuffer[Blocksize];
    unsigned char CurrentBlock[BlockData];
    unsigned int ptr = 1;
    unsigned char i;
    const byte fsc = FRAMESTART;

    if (!st->send || (st->send(&fsc, 1) != 1))
    {
        return 0;
    }

    for (i = 0; i < BlockNumber; i++)
    {
        if ((unsigned int) (length - ptr + 1) >= BlockData)
        {
            MEMCPYFUNCTION(CurrentBlock, &buffer[ptr], BlockData);
            ptr += BlockData;
        }
        else
        {
            const unsigned char zeroes[BlockData] =
            { 0 };
            MEMCPYFUNCTION(CurrentBlock, zeroes, BlockData);
            MEMCPYFUNCTION(CurrentBlock, &buffer[ptr], length - ptr + 1);
            ptr = length;
        }
        encode_data(CurrentBlock, BlockData, newBuffer, st->ecc);
        if(st->send(newBuffer, Blocksize) != Blocksize)
            return 0;
        if (ptr == length)
            break;
    }
    return BlockNumber * BlockData + 1;
}
#endif
/************************************************************************
 * Initialize the smp-buffers
 * Set:
 * 		st->buffer
 * 		st->send
 * 		st->frameReadyCallback
 * 		st->rogueFrameCallback
 * 		and if compiling with RS_Code support
 * 		st->ecc
 * before calling this function.
 * Return -1 if the datasize of the buffer is not the sizeof(uin8_t)
 * 0 otherwise

 ************************************************************************/
signed char SMP_Init(smp_struct_t* st)
{
#ifdef USE_RS_CODE
    if (st->ecc)
    {
        initialize_ecc(st->ecc);
    }
    st->rsPtr = 0;
#endif // USE_RS_CODE
    st->flags.recieving = 0;
    st->crc = 0;
    st->flags.status = 0;
    return 0;
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

unsigned int SMP_estimatePacketLength(const byte* buffer, unsigned short length, smp_struct_t *st)
{
    unsigned short overheadCounter = 0;
    unsigned int ret = 0;
    unsigned short i;
    for(i = 0; i < length; i++)
    {
        if(buffer[i] == FRAMESTART)
            overheadCounter++;
    }
    ret = length + overheadCounter + 5;
#ifdef USE_RS_CODE
    if(st->ecc)
    {
        unsigned char Blocknumber = ret/BlockData;
        ret = ret * Blocksize;
        if((double)ret/(double)BlockData > Blocknumber)
            ret += Blocksize;
    }
#endif
    return ret;
}

unsigned short SMP_EncodeRS(const byte* buffer, unsigned short length, smp_struct_t *st)
{
    if(st->ecc == 0)
        return 0;
    #ifdef USE_RS_CODE
    int Framestart = seekFramestart(buffer,length);
    int signedLength = length;
    const byte* bufferPtr = buffer;
    unsigned char offset = 0;
    unsigned short packetLength;

    unsigned int sendBytes = 0;
    unsigned int truePacketLength = 0;
    unsigned int i;

    while(Framestart >= 0)
    {
        signedLength -= Framestart;
        if(signedLength < 0)
            return sendBytes;
        bufferPtr = &bufferPtr[Framestart];
        offset = 0;

        //Get PacketLength

        packetLength = bufferPtr[1];
        if(bufferPtr[1] == FRAMESTART)
            offset++;
        packetLength |= bufferPtr[2+offset] << 8;
        offset = 0;
        for(i = 1; i <= packetLength; i++)
        {
            if(bufferPtr[i] == FRAMESTART)
            {
                i++; //Skip mask byte
                packetLength++;
            }
        }
        truePacketLength = packetLength + 1; //Calculate Framestart to true length;

        if(length < truePacketLength)
            return sendBytes;
        if(buffer[truePacketLength] == FRAMESTART)
            truePacketLength += 2;
        else
            truePacketLength++;

        if(length < truePacketLength)
            return sendBytes;

        if(buffer[truePacketLength] == FRAMESTART)
            truePacketLength += 2;
        else
            truePacketLength++;

        if(signedLength < truePacketLength)
            return sendBytes; //No full packet in buffer

        //Encode and send packet
        if(SMP_internal_EncodeRS(bufferPtr,truePacketLength,st) > 0)
        {
            sendBytes += Framestart + truePacketLength;
        }
        else
        {
            return sendBytes;
        }

        signedLength -= truePacketLength;
        if(signedLength < 0)
            return sendBytes;
        bufferPtr = &bufferPtr[truePacketLength];
        Framestart = seekFramestart(bufferPtr,signedLength);
    }
    return sendBytes;
    #else
        return 0;
    #endif
}

/************************************************************************/
/* Sends data to the smp outputbuffer                                   */

/************************************************************************/
unsigned short SMP_Send(const byte *buffer, unsigned short length, byte RSEncode, smp_struct_t *st)
{
    unsigned int i = 2;
    unsigned int offset = 0;
    unsigned short crc = 0;

    if (length > MAX_PAYLOAD)
        return 0;

    unsigned char message[2 * (length + 2) + 5];
    unsigned char* messagePtr = &message[5];

    for (i = 0; i < length; i++)
    {

        if (buffer[i] == FRAMESTART)
        {
            messagePtr[i + offset] = FRAMESTART;
            offset++;
        }

        messagePtr[i + offset] = buffer[i];
        crc = crc16(crc, buffer[i], CRC_POLYNOM);
    }

    messagePtr[i + offset] = crc >> 8; //CRC high byte
    if (messagePtr[i + offset] == FRAMESTART)
    {
        offset++;
        messagePtr[i + offset] = FRAMESTART;
    }
    messagePtr[i + offset + 1] = crc & 0xFF; //CRC low byte
    if (messagePtr[i + offset + 1] == FRAMESTART)
    {
        offset++;
        messagePtr[i + offset + 1] = FRAMESTART;
    }

    unsigned int packageSize = length + 2;
    unsigned int completeFramesize = packageSize + offset;
    unsigned char HeaderSize = 3;
    unsigned char header[5];
    offset = 0;
    header[0] = FRAMESTART;
    header[1] = packageSize & 0xFF;
    if (header[1] == FRAMESTART)
    {
        header[2] = FRAMESTART;
        HeaderSize++;
        offset = 1;
    }
    header[2 + offset] = packageSize >> 8;
    if (header[2 + offset] == FRAMESTART)
    {
        header[3 + offset] = FRAMESTART;
        HeaderSize++;
        offset = 2;
    }

    messagePtr = &message[5 - HeaderSize];
    MEMCPYFUNCTION(messagePtr,header,HeaderSize);

    unsigned int messageSize = completeFramesize + 3 + offset;
#ifdef USE_RS_CODE
    if(RSEncode && (st->ecc != 0))
    {
        if(SMP_internal_EncodeRS(messagePtr,messageSize,st) > 0)
            return length;
    }
    else
    {
        if(st->intermediateSend)
            return (st->intermediateSend(messagePtr, messageSize) == (messageSize)) ? length : 0;
        else if(st->send)
            return (st->send(messagePtr, messageSize) == (messageSize)) ? length : 0;
    }
#else
        if(st->send)
            return (st->send(messagePtr, messageSize) == (messageSize)) ? length : 0;
#endif
    return 0;
}

/************************************************************************

 Recieve multiple bytes from the Interface and writes it to the
 recievebuffer.

 ************************************************************************/

inline signed char SMP_RecieveInBytes(const byte* data, unsigned int length, smp_struct_t* st)
{
    unsigned int i;
    signed char ret = 0;
    signed char smpRet;
    for (i = 0; i < length; i++)
    {
        smpRet = SMP_RecieveInByte(data[i], st);
        if(smpRet != 0)
            ret = smpRet;
    }
    return ret;
}

signed char private_SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    switch (st->flags.status)
        //State machine
    {
    case 0: //Idle State Waiting for Framestart
        break;
    case 1:
        if (!st->bytesToRecieve)
        {
            st->bytesToRecieve = data;
        }
        else
        {
            st->bytesToRecieve |= data << 8;
            st->flags.status = 2;
            fifo_clear(st->buffer);
            st->crc = 0;
        }
        break;
    case 2:
        if (st->bytesToRecieve == 2) //Only CRC to recieve
        {
            st->flags.status = 3;
        }
        else
        {
            st->bytesToRecieve--;
            if (!fifo_write_byte(data, st->buffer))
            {
                //Bufferoverflow
                st->flags.status = 0;
                st->flags.recieving = 0;
#ifdef USE_RS_CODE
                st->rsPtr = 0;
#endif
                return -1;
            }
            st->crc = crc16(st->crc, data, CRC_POLYNOM);
            break;
        }
    case 3:
        if (!st->bytesToRecieve)
        {
            st->flags.status = 0;
            st->flags.recieving = 0;
#ifdef USE_RS_CODE
            st->rsPtr = 0;
#endif
            if (st->crc == ((unsigned short) (st->crcHighByte << 8) | data)) //Read the crc and compare
            {
                //Data ready
                if (st->frameReadyCallback)
                    return st->frameReadyCallback(st->buffer);
                else
                    return -2;
            }
            else //crc doesnt match.
            {
                if (st->rogueFrameCallback)
                    return st->rogueFrameCallback(st->buffer);
            }
        }
        else
        {
            st->crcHighByte = data; //Save first byte of crc
            st->bytesToRecieve = 0;
        }
        break;

    default: //Invalid State
        st->flags.status = 0;
        st->bytesToRecieve = 0;
        st->flags.recieving = 0;
#ifdef USE_RS_CODE
        st->rsPtr = 0;
#endif // USE_RS_CODE
        return -3;
    }
    return 0;
}

/************************************************************************/
/* Recieve a byte from the interface                                    */

/************************************************************************/
#ifdef USE_RS_CODE
signed char private_SMP_RecieveInByte_2(byte data, smp_struct_t* st)
#else
signed char SMP_RecieveInByte(byte data, smp_struct_t* st)
#endif // USE_RS_CODE
{
    if (data == FRAMESTART)
    {
        if (st->flags.recievedDelimeter)
        {
            st->flags.recievedDelimeter = 0;
            return private_SMP_RecieveInByte(data, st);
        }
        else
            st->flags.recievedDelimeter = 1;
    }
    else
    {
        if (st->flags.recievedDelimeter)
        {
            st->flags.status = 1;
            st->bytesToRecieve = 0;
            st->flags.recievedDelimeter = 0;
            if (st->flags.recieving)
            {
                if (st->rogueFrameCallback)
                    st->rogueFrameCallback(st->buffer);
            }
            st->flags.recieving = 1;
        }
        return private_SMP_RecieveInByte(data, st);
    }
    return 0;
}

#ifdef USE_RS_CODE

signed char SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    unsigned int i;
    signed char ret = 0;
    signed char smpRet;
    if ((st->flags.recieving || st->flags.recievedDelimeter) && st->ecc)
    {
        st->rsBuffer[st->rsPtr] = data;
        st->rsPtr++;
        if (st->rsPtr >= Blocksize)
        {
            st->rsPtr = 0;
            decode_data(st->rsBuffer, Blocksize, st->ecc);
            if (check_syndrome(st->ecc) != 0)
            {
                if (!correct_errors_erasures(st->rsBuffer, Blocksize, 0, 0, st->ecc))
                {
                    st->flags.status = 0;
                    st->bytesToRecieve = 0;
                    st->flags.recieving = 0;
                    int Framepos = seekFramestart(st->rsBuffer,
                                                           Blocksize);
                    if (Framepos >= 0)
                    {
                        private_SMP_RecieveInByte_2(st->rsBuffer[Framepos], st);
                        unsigned int cpyLength = Blocksize - Framepos - 1;
                        byte newBlock[cpyLength];
                        MEMCPYFUNCTION(&st->rsBuffer[Framepos + 1], newBlock, cpyLength);
                        MEMCPYFUNCTION(newBlock, st->rsBuffer, cpyLength);
                        st->rsPtr = cpyLength;
                        return -4;
                    }
                    return -5;
                }
            }
            for (i = 0; i < BlockData; i++)
            {
                smpRet = private_SMP_RecieveInByte_2(st->rsBuffer[i], st);
                if(smpRet != 0)
                    ret = smpRet;
            }
        }
    }
    else
    {
        return private_SMP_RecieveInByte_2(data, st);
    }
    return ret;
}
#endif // USE_RS_CODE

/**********************************************************************
 * Get the amounts of byte to recieve from the Interface for a full frame
 * This value is only valid if status.recieving is true
 * However if status.recieving is false this value should hold 0 but this
 * isn't confirmed yet
 **********************************************************************/
byte SMP_GetBytesToRecieve(smp_struct_t* st)
{
    if (st->flags.recieving)
        return st->bytesToRecieve;
    else
        return 0;
}

/**
 * Returns true if the smp stack for the smp_struct object is recieving
 * @param st
 * @return
 */
byte SMP_IsRecieving(smp_struct_t* st)
{
    return st->flags.recieving;
}
