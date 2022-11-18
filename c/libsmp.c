/*****************************************************************************************************
 File: libsmp
 Autor: Peter Kremsner
 Date: 12.9.2014

 Basic data transmission protocoll to ensure data integrity

 ******************************************************************************************************/
#include "libsmp.h"
#include <string.h>

#define MAX_PAYLOAD 65534

// Helper macro to get the size of a nested struct
#define sizeof_field(s, m) (sizeof((((s *)0)->m)))

/***********************************************************************
 * @brief Private function definiton to calculate the crc checksum
 ***********************************************************************/
MODULE_API uint16_t SMP_crc16(uint16_t crc, uint16_t c, uint16_t mask)
{
    uint8_t i;
    uint16_t _crc = crc;
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
    if (preserveReceivedDelimeter)
    {
        receivedDelimeter = smp->flags.recievedDelimeter;
    }
    if (preserveReceivedDelimeter)
    {
        smp->flags.recievedDelimeter = receivedDelimeter;
    }
}

/************************************************************************
 * @brief Initialize the smp-buffers
 ************************************************************************/
signed char SMP_Init(smp_struct_t *st)
{
    SMP_ResetDecoderState(st, false);
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
    ret = length + overheadCounter + 10;
    return ret;
}

/**
 * @brief Calculates the minimum required sendbuffer length for Send function to work properly
 * */
MODULE_API uint32_t SMP_CalculateMinimumSendBufferSize(unsigned short length)
{
    return MINIMUM_SMP_BUFFERLENGTH(length);
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
        crc = SMP_crc16(crc, buffer[i], CRC_POLYNOM);
    }

    messagePtr[i + offset] = crc >> 8; // CRC high byte
    if (messagePtr[i + offset] == FRAMESTART)
    {
        offset++;
        messagePtr[i + offset] = FRAMESTART;
    }
    messagePtr[i + offset + 1] = crc & 0xFF; // CRC low byte
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

MODULE_API uint16_t SMP_PacketGetLength(const byte *data, uint16_t *headerlength)
{
    uint16_t protocolbytecounter = 1;    // Initialize with 1 we count the framestart here
    const uint8_t *lengthptr = data + 1; // Skip Framestart
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
    if (headerlength)
    {
        *headerlength = protocolbytecounter;
    }
    return length;
}

MODULE_API bool SMP_PacketValid(const byte *data, uint16_t packetlength, uint16_t headerlength, uint16_t *crclength)
{
    uint16_t crc = 0;
    uint16_t crccount = 0;
    const uint8_t *payload = data + headerlength;
    // We only need to check the checksum to validate the data integrety
    uint16_t payloadlength = packetlength - headerlength;
    for (uint16_t i = 0; i < payloadlength - 2; i++)
    {
        crc = SMP_crc16(crc, *payload, CRC_POLYNOM);
        if (*payload == FRAMESTART)
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
            // Bytestufferror
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
            // Bytestufferror
            return false;
        }
    }
    if (crclength)
    {
        *crclength = crccount;
    }
    return crc == transmittedCRC;
}

/**
 *  @brief Private function to process the received bytes without the bytestuffing
 * **/
static smp_decoder_stat private_SMP_RecieveInByte(byte data, byte* decoded, smp_struct_t *st)
{
    switch (st->flags.decoderstate)
    // State machine
    {
    case 0: // Idle State Waiting for Framestart
        st->flags.lengthreceived = 0;
        return NO_PACKET_START;
    case 1:
        st->flags.recieving = 1;
        if (!st->flags.lengthreceived)
        {
            st->bytesToRecieve = data;
            st->flags.lengthreceived = 1;
        }
        else
        {
            st->bytesToRecieve |= data << 8;
            st->flags.decoderstate = 2;
            st->crc = 0;
        }
        return PACKET_START_FOUND;
    case 2:
        st->bytesToRecieve--;
        *decoded = data;
        st->crc = SMP_crc16(st->crc, data, CRC_POLYNOM);
        if (st->bytesToRecieve == 2) // If we only have two bytes to receive we switch to the reception of the crc data
        {
            st->flags.decoderstate = 3;
        }
        else if (st->bytesToRecieve < 2)
        {
            // This should not happen and indicates an memorycorruption
            SMP_ResetDecoderState(st, true);
            return ERROR_UNKOWN;
        }
        return RECEIVED_BYTE;
    case 3:
        if (st->bytesToRecieve)
        {
            st->crcHighByte = data; // At first we save the high byte of the transmitted crc
            st->bytesToRecieve = 0; // Set bytestoReceive to 0 to signal the receiving of the last byte
        }
        else
        {
            uint16_t transmittedCRC = (uint16_t)(st->crcHighByte << 8) | data;
            if (st->crc == transmittedCRC) // Read the crc and compare
            {
                SMP_ResetDecoderState(st, false);
                return PACKET_READY;
            }
            else // crc doesnt match.
            {
                SMP_ResetDecoderState(st, true);
                return CRC_ERROR;
            }
        }
        return RECEIVE_CRC;

    default: // Invalid State
        SMP_ResetDecoderState(st, true);
        return ERROR_UNKOWN;
    }
    return ERROR_UNKOWN;
}

/************************************************************************
 * @brief Parser received byte
 * Call this function on every byte received from the interface
 * This function returns a value that indicates errors or if a packet was received:
 * >1: Packet received. The returnvalue is the return value of the callback function + 2
 * 1: Packet received but no callback function specified
 * 0: No action
 * -1: Bufferoverflow when receiving
 * -2: Possible memorycorruption error
 * -3: Reserved
 * -4: CRC Error
 ************************************************************************/
MODULE_API smp_decoder_stat SMP_RecieveInByte(byte data, byte* decoded, smp_struct_t *st)
{
    smp_decoder_stat ret = NO_PACKET_START;
    // Remove the bytestuffing from the data
    if (data == FRAMESTART)
    {
        if (st->flags.recievedDelimeter)
        {
            st->flags.recievedDelimeter = 0;
            ret = private_SMP_RecieveInByte(data, decoded, st);
        }
        else
        {
            st->flags.recievedDelimeter = 1;
            ret = RECEIVING;
        }
    }
    else
    {
        if (st->flags.recievedDelimeter)
        {
            if (st->flags.recieving)
            {
                ret = REPEATED_FRAMESTART;
            }
            SMP_ResetDecoderState(st, false);
            st->flags.decoderstate = 1; // Set the decoder into receive mode
            st->flags.recieving = 1;
        }
        st->flags.recievedDelimeter = 0;
        smp_decoder_stat recieveReturnValue = private_SMP_RecieveInByte(data, decoded, st);
        if(ret != REPEATED_FRAMESTART)
        {
            ret = recieveReturnValue;
        }
    }
    return ret;
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
        return st->bytesToRecieve;
    else
        return 0;
}

/**
 * @brief Returns true if the smp stack for the smp_struct object is recieving
 */
MODULE_API bool SMP_IsRecieving(smp_struct_t *st)
{
    return st->flags.recieving;
}
