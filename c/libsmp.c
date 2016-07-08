/*****************************************************************************************************
 File: libsmp
 Autor: Peter Kremsner
 Date: 12.9.2014

 Basic data transmission protocoll to ensure data integrity

 ******************************************************************************************************/
#include "libsmp.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PAYLOAD 65535

/***********************************************************************
 * Private function definiton to calculate the crc checksum
 */
unsigned int crc16(unsigned int crc, unsigned int c, unsigned int mask)
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

/************************************************************************/
/* Initialize the smp-buffers
 * Return -1 if the datasize of the buffer is not the sizeof(uin8_t)
 * 0 otherwise

 /************************************************************************/
signed char SMP_Init(smp_struct_t* st, fifo_t* buffer, SMP_send_function send, SMP_Frame_Ready frameReadyCallback, SMP_Frame_Ready rogueFrameCallback)
{
    st->buffer = buffer;
    if (buffer->objectSize != sizeof(uint8_t))
        return -1;
    st->flags.recieving = 0;
    st->flags.error = 0;
    st->crc = 0;
    st->flags.status = 0;
    st->send = send;
    st->frameReadyCallback = frameReadyCallback;
    st->rogueFrameCallback = rogueFrameCallback;
    return 0;
}

/************************************************************************/
/* Sends data to the smp outputbuffer                                   */

/************************************************************************/
unsigned char SMP_Send(byte *buffer, unsigned short length,smp_struct_t *st)
{
    unsigned short i = 2;
    unsigned short offset = 0;
    unsigned short crc = 0;

    if (length > MAX_PAYLOAD)
        return 0;

    unsigned char message1[2*(length + 2)];
    unsigned char message2[2*(length + 2) + 5];

    for (i = 0; i < length; i++)
    {

        if (buffer[i] == FRAMESTART)
        {
            message1[i + offset] = FRAMESTART;
            offset++;
        }

        message1[i + offset] = buffer[i];

        crc = crc16(crc, buffer[i], CRC_POLYNOM);
    }

    message1[i + offset] = crc >> 8; //CRC high byte
    if(message1[i + offset] == FRAMESTART)
    {
        offset++;
        message1[i + offset] = FRAMESTART;
    }
    message1[i + offset + 1] = crc & 0xFF; //CRC low byte
    if(message1[i + offset + 1] == FRAMESTART)
    {
        offset++;
        message1[i + offset + 1] = FRAMESTART;
    }
    unsigned int packageSize = length + 2;
    unsigned int completeFramesize = packageSize + offset;
    offset = 0;
    message2[0] = FRAMESTART;
    message2[1] = packageSize & 0xFF;
    if(message2[1] == FRAMESTART)
    {
        message2[2] = FRAMESTART;
        offset = 1;
    }
    message2[2+offset] = packageSize >> 8;
    if(message2[2+offset] == FRAMESTART)
    {
        message2[3+offset] = FRAMESTART;
        offset = 2;
    }

    for(i = 3 + offset; i < completeFramesize + 3 + offset; i++)
    {
        message2[i] = message1[i - (3 + offset)];
    }

    if(st->send)
        return st->send(message2,completeFramesize + 3 + offset);
    else
        return 0;
}

/************************************************************************

 Recieve multiple bytes from the Interface and writes it to the
 recievebuffer.

 ************************************************************************/

inline signed char SMP_RecieveInBytes(byte* data, unsigned int length, smp_struct_t* st)
{
    unsigned int i;
    signed char ret;
    for (i = 0; i < length; i++)
    {
        ret = SMP_RecieveInByte(data[i], st);
        if (ret < 0)
            break;
    }
    return ret;
}

signed char private_SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    switch (st->flags.status) //State machine
    {
    case 0: //Idle State Waiting for Framestart
        break;
    case 1:
        st->flags.recieving = 1;
        if(!st->bytesToRecieve)
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
            if (!fifo_write_object(&data, st->buffer))
            {
                //Bufferoverflow
                st->flags.status = 0;
                st->flags.recieving = 0;
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
            if (st->crc == ((unsigned short) (st->crcHighByte << 8) | data)) //Read the crc and compare
            {
                //Data ready
                if(st->frameReadyCallback)
                    return st->frameReadyCallback(st->buffer);
                else
                    return -2;
            }
            else //crc doesnt match.
            {
                if(st->rogueFrameCallback)
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
        break;
    }
    return 0;
}

/************************************************************************/
/* Recieve a byte from the interface                                    */

/************************************************************************/
signed char SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    if(data == FRAMESTART)
    {
        if(st->flags.recievedDelimeter)
        {
            st->flags.recievedDelimeter = 0;
            return private_SMP_RecieveInByte(data,st);
        }
        else
            st->flags.recievedDelimeter = 1;
    }
    else
    {
        if(st->flags.recievedDelimeter)
        {
            st->flags.status = 1;
            st->bytesToRecieve = 0;
            st->flags.recievedDelimeter = 0;
            if(st->flags.recieving)
            {
                if(st->rogueFrameCallback)
                    st->rogueFrameCallback(st->buffer);
            }
        }
        return private_SMP_RecieveInByte(data,st);
    }
    return 0;
}

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
