/*****************************************************************************************************
 File: libsmp
 Autor: Peter Kremsner
 Date: 12.9.2014
 
 Basic data transmission protocoll to ensure data integrity
 
 ******************************************************************************************************/
#include "libsmp.h"
#include <string.h>

#ifdef CREATE_ALLOC_LAYER
#include <stdlib.h>
#endif

#define MAX_PAYLOAD 65534

//Helper macro to get the size of a nested struct
#define sizeof_field(s, m) (sizeof((((s *)0)->m)))

/***********************************************************************
 * @brief Private function definiton to calculate the crc checksum
 ***********************************************************************/
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

void SMP_ResetDecoderState(smp_struct_t *smp, bool preserveReceivedDelimeter)
{
    bool receivedDelimeter = false;
    if(preserveReceivedDelimeter)
    {
        receivedDelimeter = smp->status.flags.recievedDelimeter;
    }
    memset(&smp->status, 0, sizeof(smp_status_t));
    smp->status.writePtr = smp->settings.buffer.buffer;
    if(preserveReceivedDelimeter)
    {
        smp->status.flags.recievedDelimeter = receivedDelimeter;
    }
}

#ifdef CREATE_ALLOC_LAYER
MODULE_API smp_struct_t *SMP_AllocateInit(smp_settings_t *settings)
{
    if (settings == 0)
    {
        return 0;
    }
    smp_struct_t *ret = malloc(sizeof(smp_struct_t));
    if (ret != 0)
    {
        SMP_Init(ret, settings);
    }
    return ret;
}

MODULE_API void SMP_Destroy(smp_struct_t *st)
{
    free(st);
}
#endif

/************************************************************************
 * @brief Initialize the smp-buffers 
 ************************************************************************/
signed char SMP_Init(smp_struct_t *st, smp_settings_t *settings)
{
    SMP_ResetDecoderState(st, false);
    if (settings != 0)
        memcpy(st, settings, sizeof(smp_settings_t)); //Copy the settings if they are passed to this function
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

MODULE_API uint32_t SMP_estimatePacketLength(const byte *buffer, unsigned short length)
{
    unsigned short overheadCounter = 0;
    unsigned int ret = 0;
    unsigned short i;
    for (i = 0; i < length; i++)
    {
        if (buffer[i] == FRAMESTART)
            overheadCounter++;
    }
    ret = length + overheadCounter + 5;
    return ret;
}

/*******************************************
 * @brief Wrapper for SMP_Send which returns the message start index instead of the pointer
 * This function returns the index at which the message starts in the messageBuffer instead
 * of a pointer to the startingposition like SMP_Send
 **/
MODULE_API unsigned int SMP_SendRetIndex(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, unsigned short *messageStartIndex)
{
    byte *messageStartPtr;
    unsigned int ret = SMP_Send(buffer, length, messageBuffer, bufferLength, &messageStartPtr);
    if (ret != 0)
    {
        *messageStartIndex = (unsigned short)(messageStartPtr - messageBuffer);
    }
    return ret;
}

/************************************************************************      
 * @brief Sends data to the smp outputbuffer
 * Create a smp packet from the data in buffer and writes it into messageBuffer
 * messageStartPtr points to the start of the smppacket
 * @return The length of the whole smp packet. If this value is zero an error
 *          occured and messageStartPtr is not valid
************************************************************************/
MODULE_API unsigned int SMP_Send(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, byte **messageStartPtr)
{
    unsigned int i = 2;
    unsigned int offset = 0;
    unsigned short crc = 0;

    if (length > MAX_PAYLOAD)
        return 0;
    unsigned char *message = messageBuffer;
    unsigned char *messagePtr = &message[5];

    if (bufferLength < (2 * (length + 2) + 5))
        return 0;

    for (i = 0; i < length; i++)
    {
        if (bufferLength <= i + offset + 5)
            return 0;
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
    memcpy(messagePtr, header, HeaderSize);

    unsigned int messageSize = completeFramesize + 3 + offset;

    *messageStartPtr = messagePtr;
    return messageSize;
}

/************************************************************************
 * @brief  Parse multiple bytes
 * This function calls SMP_RecieveInByte for every byte in the buffer
 ************************************************************************/

MODULE_API inline signed char SMP_RecieveInBytes(const byte *data, uint32_t length, smp_struct_t *st)
{
    unsigned int i;
    signed char ret = 0;
    signed char smpRet;
    for (i = 0; i < length; i++)
    {
        smpRet = SMP_RecieveInByte(data[i], st);
        if (smpRet != 0)
            ret = smpRet;
    }
    return ret;
}

MODULE_API uint16_t SMP_PacketGetLength(const byte *data, uint16_t* headerlength)
{
    uint16_t protocolbytecounter = 1; //Initialize with 1 we count the framestart here
    const uint8_t *lengthptr = data + 1; //Skip Framestart
    uint16_t length = *lengthptr;
    protocolbytecounter++;
    if (*lengthptr == FRAMESTART)
    {
        protocolbytecounter++;
        if (*lengthptr != FRAMESTART)
            return 0;
        lengthptr++;
    }
    lengthptr++;
    protocolbytecounter++;
    length |= *lengthptr << 8;
    if (*lengthptr == FRAMESTART)
    {
        protocolbytecounter++;
        lengthptr++;
        if (*lengthptr != FRAMESTART)
            return 0;
    }
    if(headerlength)
    {
        *headerlength = protocolbytecounter;
    }
    return length;
}

MODULE_API bool SMP_PacketValid(const byte *data, uint16_t packetlength, uint16_t headerlength, uint16_t* crclength)
{
    uint16_t crc = 0;
    uint16_t crccount = 0;
    const uint8_t *payload = data + headerlength;
    //We only need to check the checksum to validate the data integrety
    uint16_t payloadlength = packetlength - headerlength;
    for (uint16_t i = 0; i < payloadlength - 2; i++)
    {
        crc = crc16(crc, *payload, CRC_POLYNOM);
        if(*payload == FRAMESTART)
        {
            payload++;
        }
        payload++;
    }
    uint16_t transmittedCRC = *payload << 8;
    crccount++;
    payload++;
    if (*payload == FRAMESTART)
    {
        crccount++;
        if (*payload != FRAMESTART)
        {
            //Bytestufferror
            return false;
        }
        payload++;
    }
    crccount++;
    transmittedCRC |= *payload;
    if (*payload == FRAMESTART)
    {
        crccount++;
        payload++;
        if (*payload != FRAMESTART)
        {
            //Bytestufferror
            return false;
        }
    }
    if(crclength)
    {
        *crclength = crccount;
    }
    return crc == transmittedCRC;
}

/**
 *  @brief Private function to process the received bytes without the bytestuffing
 * **/
MODULE_API signed char private_SMP_RecieveInByte(byte data, smp_struct_t *st)
{
    switch (st->status.flags.decoderstate)
    //State machine
    {
    case 0: //Idle State Waiting for Framestart
        st->status.flags.lengthreceived = 0;
        break;
    case 1:
        if (!st->status.flags.lengthreceived)
        {
            st->status.bytesToRecieve = data;
            st->status.flags.lengthreceived = 1;
        }
        else
        {
            st->status.bytesToRecieve |= data << 8;
            st->status.flags.decoderstate = 2;
            st->status.crc = 0;
            st->status.writePtr = st->settings.buffer.buffer;
        }
        break;
    case 2:
        st->status.bytesToRecieve--;
        if (st->status.writePtr >= (st->settings.buffer.buffer + st->settings.buffer.maxlength))
        {
            //We have a bufferoverflow
            st->status.flags.decoderstate = 0;
            st->status.flags.recieving = 0;
            return -1; //Show error by returning -1
        }
        *st->status.writePtr = data;
        st->status.writePtr++;
        st->status.crc = crc16(st->status.crc, data, CRC_POLYNOM);
        if (st->status.bytesToRecieve == 2) //If we only have two bytes to receive we switch to the reception of the crc data
        {
            st->status.flags.decoderstate = 3;
        }
        else if (st->status.bytesToRecieve < 2)
        {
            //This should not happen and indicates an memorycorruption
            st->status.flags.decoderstate = 0;
            st->status.bytesToRecieve = 0;
            st->status.flags.recieving = 0;
            return -2;
        }
        break;
    case 3:
        if (st->status.bytesToRecieve)
        {
            st->status.crcHighByte = data; //At first we save the high byte of the transmitted crc
            st->status.bytesToRecieve = 0; //Set bytestoReceive to 0 to signal the receiving of the last byte
        }
        else
        {
            unsigned char ret = -4;
            if (st->status.crc == ((unsigned short)(st->status.crcHighByte << 8) | data)) //Read the crc and compare
            {
                //Data ready
                if (st->settings.frameReadyCallback)
                {
                    ret = st->settings.frameReadyCallback(st->settings.buffer.buffer, st->status.writePtr - st->settings.buffer.buffer);
                }
                else
                {
                    ret = -3;
                }
            }
            else //crc doesnt match.
            {
                if (st->settings.rogueFrameCallback)
                {
                    ret = st->settings.rogueFrameCallback(st->settings.buffer.buffer, st->status.writePtr - st->settings.buffer.buffer);
                }
            }
            SMP_ResetDecoderState(st, true);
            return ret;
        }
        break;

    default: //Invalid State
        SMP_ResetDecoderState(st, true);
        return -5;
    }
    return 0;
}

/************************************************************************
* @brief Parser received byte
* Call this function on every byte received from the interface
************************************************************************/
MODULE_API signed char SMP_RecieveInByte(byte data, smp_struct_t *st)
{
    // Remove the bytestuffing from the data
    if (data == FRAMESTART)
    {
        if (st->status.flags.recievedDelimeter)
        {
            st->status.flags.recievedDelimeter = 0;
            return private_SMP_RecieveInByte(data, st);
        }
        else
        {
            st->status.flags.recievedDelimeter = 1;
        }
    }
    else
    {
        if (st->status.flags.recievedDelimeter)
        {
            if (st->status.flags.recieving)
            {
                if (st->settings.rogueFrameCallback)
                    st->settings.rogueFrameCallback(st->settings.buffer.buffer, st->status.writePtr - st->settings.buffer.buffer);
            }
            SMP_ResetDecoderState(st, false);
            st->status.flags.decoderstate = 1; //Set the decoder into receive mode
            st->status.flags.recieving = 1;
        }
        st->status.flags.recievedDelimeter = 0;
        return private_SMP_RecieveInByte(data, st);
    }
    return 0;
}

/**********************************************************************
 * @brief Get the amounts of byte to recieve from the Interface for a full frame
 * This value is only valid if status.recieving is true
 * However if status.recieving is false this value should hold 0 but this
 * isn't confirmed yet
 **********************************************************************/
MODULE_API uint32_t SMP_GetBytesToRecieve(smp_struct_t *st)
{
    if (SMP_IsRecieving(st))
        return st->status.bytesToRecieve;
    else
        return 0;
}

/**
 * @brief Returns true if the smp stack for the smp_struct object is recieving
 */
MODULE_API bool SMP_IsRecieving(smp_struct_t *st)
{
    return st->status.flags.recieving;
}
