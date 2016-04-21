/*****************************************************************************************************

Autor: Peter Kremsner
Date: 12.9.2014

 ******************************************************************************************************/
#include "smp.h"

#define TRUE 1
#define FALSE 0

static SMPframeReady FrameCallback = 0x00;

unsigned int crc16(unsigned int crc, unsigned int c, unsigned int mask) {
    unsigned char i;
    unsigned int _crc = crc;
    for (i = 0; i < 8; i++) {
        if ((_crc ^ c) & 1) {
            _crc = (_crc >> 1)^mask;
        } else _crc >>= 1;
        c >>= 1;
    }
    return (_crc);
}

/************************************************************************/
/* Initialize the smp-buffers                                           */

/************************************************************************/
void SMP_Init(smp_struct_t* st)
{
    st->flags.recieving = 0;
    st->flags.error = 0;
    st->crc = 0;
    st->flags.status = 0;
}

/************************************************************************/
/* Register a function as callback if a full frame is recieved          */

/************************************************************************/
void SMP_RegisterFrameReadyCallback(SMPframeReady func) {
    FrameCallback = func;
}

/************************************************************************/
/* Unregisters the callback function                                    */

/************************************************************************/
void SMP_UnregisterFrameReadyCallback(void) {
    FrameCallback = 0x00;
}

/************************************************************************/
/* Sends data to the smp outputbuffer                                   */

/************************************************************************/
unsigned char SMP_Send(byte *buffer, byte length, byte* smpBuffer, byte smpBuffersize) {
    unsigned short i = 2;
    unsigned char offset = 0;
    unsigned short crc = 0;

    if (length > MAX_PAYLOAD)
        return 0;

    if(smpBuffersize < 4)
        return 0;

    smpBuffer[0] = FRAMESTART; //Startdelimeter

    for (i = 2; i < (unsigned short)(length + 2); i++) {

        if(smpBuffersize < i + offset + 1)
            return 0;

        if (buffer[i - 2] == FRAMESTART) {
            smpBuffer[i + offset] = FRAMESTART;
            offset++;
            if(smpBuffersize < i + offset + 1)
                return 0;
        }

        smpBuffer[i + offset] = buffer[i - 2];

        crc = crc16(crc, buffer[i - 2], CRC_POLYNOM);
    }

    if(smpBuffersize < i + offset + 2)
        return 0;

    smpBuffer[i + offset] = crc >> 8; //CRC high byte
    i++;
    smpBuffer[i + offset] = crc & 0xFF; //CRC low byte

    smpBuffer[1] = i + offset - 1; //calc length field
    return i + offset + 1;
}

/************************************************************************

   Recieve multiple bytes from the Interface and writes it to the
   recievebuffer.

 ************************************************************************/

inline smp_error_t SMP_RecieveInBytes(byte* data, byte length, smp_struct_t* st) {
    int i;
	smp_error_t error;
    for (i = 0; i < length; i++) {
		error = SMP_RecieveInByte(data[i],st);
        if(error.errorCode)
			return error;
    }
	error.errorCode = 0;
	return error;
}

/************************************************************************/
/* Recieve a byte from the interface                                    */

/************************************************************************/
inline smp_error_t SMP_RecieveInByte(byte data, smp_struct_t* st) {
	smp_error_t error;
	error.errorCode = 0;
    switch (st->flags.status) //State machine
    {
        case 0:
            //Detect Framestart
            if (data == FRAMESTART)
            {
                st->flags.status = 1;
                st->flags.recieving = TRUE;
            }
            break;
        case 1:
            CASE_1_LABEL :
            if (data == FRAMESTART)
                break;
            if (data > MAX_PAYLOAD + 2)
                st->flags.status = 0; //length greater than MAX_PAYLOAD is not allowed
            else
            {
                st->bytesToRecieve = data;
                st->flags.status = 2;
                st->buffer.ptr = 0;
                st->crc = 0;
                st->flags.recievedDelimeter = 0;
            }
            break;
        case 2:
            if (st->bytesToRecieve == 2) {
                st->flags.status = 3;
            } else {
                st->bytesToRecieve--;
                if (st->flags.recievedDelimeter) {
                    if (data != FRAMESTART) //Not escaped framestart handle as real Framestart
                    {
                        st->bytesToRecieve = data;
                    } else {
                        //Valid data as FRAMESTART character
                        st->buffer.Message[st->buffer.ptr++] = data;
                        st->crc = crc16(st->crc, data, CRC_POLYNOM);
                    }
                    st->flags.recievedDelimeter = 0;
                } else {
                    if (data == FRAMESTART) {
                        st->flags.recievedDelimeter = 1;
                    } else {
                        st->buffer.Message[st->buffer.ptr++] = data;
                        st->crc = crc16(st->crc, data, CRC_POLYNOM);
                    }
                }
                break;
            }
        case 3:
            if (!st->bytesToRecieve) {
                if (st->crc == ((unsigned short) (st->crcHighByte << 8) | data)) //Read the crc and compare
                {
                    //Data ready
                    if (FrameCallback) {
                        return FrameCallback(st->buffer.ptr, st->buffer.Message);
                    }
                } else //crc doesnt match.
                {
                    if (st->crcHighByte == FRAMESTART) //If the crc doesnt match and the high byte is the framestart delimeter
                    { //expect that this was the start of a new frame.
                        goto CASE_1_LABEL;
                    }
                    else if(data == FRAMESTART) //Lowbyte of crc matches Framestart.
                    {
                        st->flags.status = 1;
                        break;
                    }
                }
                st->flags.status = 0;
                st->flags.recieving = 0;
            } else {
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

/**********************************************************************
 * Get the amounts of byte to recieve from the Interface for a full frame
 * This value is only valid if status.recieving is true
 * However if status.recieving is false this value should hold 0 but this
 * isn't confirmed yet
 **********************************************************************/
byte SMP_GetBytesToRecieve(smp_struct_t* st) {
    if(st->flags.recieving)
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
    if(FrameCallback)
        return FrameCallback(0,0);
    else
        return error;
}
