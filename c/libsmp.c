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
char SMP_Init(smp_struct_t* st, fifo_t* buffer, smp_send_function send, SMPframeReady frameReadyCallback)
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
    return 0;
}

/************************************************************************/
/* Sends data to the smp outputbuffer                                   */

/************************************************************************/
unsigned char SMP_Send(byte *buffer, byte length,smp_struct_t *st)
{
    unsigned short i = 2;
    unsigned char offset = 0;
    unsigned short crc = 0;

    if (length > MAX_PAYLOAD)
        return 0;

    unsigned char message1[MAX_FRAMESIZE];
    unsigned char message2[MAX_FRAMESIZE];

    for (i = 0; i < length; i++)
    {

        if (buffer[i] == FRAMESTART)
        {
            message1[i + offset] = FRAMESTART;
            offset++;
            if (MAX_FRAMESIZE < i + offset + 1)
                return 0;
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
    i++;
    message1[i + offset] = crc & 0xFF; //CRC low byte
    if(message1[i + offset] == FRAMESTART)
    {
        offset++;
        message1[i + offset] = FRAMESTART;
    }

    unsigned short packageSize = i + offset - 1;
    offset = 0;
    message2[0] = FRAMESTART;
    message2[1] = packageSize >> 8;
    if(message2[1] == FRAMESTART)
    {
        message2[2] = FRAMESTART;
        offset = 1;
    }
    message2[2+offset] = packageSize && 0xFF;
    if(message2[2+offset] == FRAMESTART)
    {
        message2[3+offset] = FRAMESTART;
        offset = 2;
    }

    for(i = 3 + offset; i < packageSize + 3 + offset; i++)
    {
        message2[i] = message1[i - (3 + offset)];
    }

    return st->send(message2,packageSize + 3 + offset);
}

/************************************************************************

 Recieve multiple bytes from the Interface and writes it to the
 recievebuffer.

 ************************************************************************/

inline smp_error_t SMP_RecieveInBytes(byte* data, byte length, smp_struct_t* st)
{
    int i;
    smp_error_t error;
    for (i = 0; i < length; i++)
    {
        error = SMP_RecieveInByte(data[i], st);
        if (error.errorCode)
            return error;
    }
    error.errorCode = 0;
    return error;
}

smp_error_t private_SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    switch (st->flags.status) //State machine
    {
    case 0:
        break;
    case 1:
CASE_1_LABEL:
        if (data == FRAMESTART)
            break;
        if (data > MAX_PAYLOAD + 2)
            st->flags.status = 0; //length greater than MAX_PAYLOAD is not allowed
        else
        {
            st->bytesToRecieve = data;
            st->flags.status = 2;
            fifo_clear(st->buffer);
            st->crc = 0;
            st->flags.recievedDelimeter = 0;
        }
        break;
    case 2:
        if (st->bytesToRecieve == 2)
        {
            st->flags.status = 3;
        }
        else
        {
            st->bytesToRecieve--;
            if (st->flags.recievedDelimeter)
            {
                if (data != FRAMESTART) //Not escaped framestart handle as real Framestart
                {
                    st->bytesToRecieve = data;
                }
                else
                {
                    //Valid data as FRAMESTART character
                    if (!fifo_write_object(&data, st->buffer))
                    {
                        //Bufferoverflow
                        error.flags.bufferFull = 1;
                        st->flags.status = 0;
                        st->flags.recieving = 0;
                    }
                    st->crc = crc16(st->crc, data, CRC_POLYNOM);
                }
                st->flags.recievedDelimeter = 0;
            }
            else
            {
                if (data == FRAMESTART)
                {
                    st->flags.recievedDelimeter = 1;
                }
                else
                {
                    if (!fifo_write_object(&data, st->buffer))
                    {
                        //Bufferoverflow
                        error.flags.bufferFull = 1;
                        st->flags.status = 0;
                        st->flags.recieving = 0;
                    }
                    st->crc = crc16(st->crc, data, CRC_POLYNOM);
                }
            }
            break;
        }
    case 3:
        if (!st->bytesToRecieve)
        {
            if (st->crc == ((unsigned short) (st->crcHighByte << 8) | data)) //Read the crc and compare
            {
                //Data ready
                return st->frameReadyCallback(st->buffer);
            }
            else //crc doesnt match.
            {
                if (st->crcHighByte == FRAMESTART) //If the crc doesnt match and the high byte is the framestart delimeter
                {
                    //expect that this was the start of a new frame.
                    goto CASE_1_LABEL;
                }
                else if (data == FRAMESTART) //Lowbyte of crc matches Framestart.
                {
                    st->flags.status = 1;
                    break;
                }
            }
            st->flags.status = 0;
            st->flags.recieving = 0;
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
    return error;
}

/************************************************************************/
/* Recieve a byte from the interface                                    */

/************************************************************************/
smp_error_t SMP_RecieveInByte(byte data, smp_struct_t* st)
{
    smp_error_t error;
    error.errorCode = 0;

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
        }
        return private_SMP_RecieveInByte(data,st);
    }
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

/**
 * Returns the current error code of the reciever
 * @return smp_error_t
 */
smp_error_t SMP_getRecieverError(void)
{
    smp_error_t error;
    error.errorCode = 0;
    if (FrameCallback)
        return FrameCallback(0);
    else
        return error;
}
