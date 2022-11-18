/*****************************************************************************************************

 Autor: Peter Kremsner
 Date: 12.9.2014

 ******************************************************************************************************/

#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "sharedlib.h"

/**
 * @brief Calculate the required size of the smp buffer (worst case) for the supplied maximum message length
 *
 */
#define MINIMUM_SMP_BUFFERLENGTH(maxmessagelength) (2 * (maxmessagelength + 2) + 5)

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAMESTART 0xFF    // The framestartdelimeter
#define CRC_POLYNOM 0xA001 // CRC Generatorpolynom

    typedef uint8_t byte;

typedef enum
{
    NO_PACKET_START,
    PACKET_START_FOUND,
    RECEIVING,
    RECEIVED_BYTE,
    RECEIVE_CRC,
    PACKET_READY,
    CRC_ERROR,
    REPEATED_FRAMESTART,
    ERROR_UNKOWN
} smp_decoder_stat;

#define SMP_SEND_BUFFER_LENGTH(messageLength) (2 * (messageLength + 2) + 5)

    // Callbacks
    // When the callbackfunction returns a negative Integer, its treated as error code.
    // When the length and the bufferpointer is both zero, then this function should return the error Code
    typedef signed char (*SMP_Frame_Ready)(uint8_t *data, uint32_t length); // FrameReadyCallback: Length is the ammount of bytes in the recieveBuffer

    /**
     * stuct to hold the status flags of the decoder
     * */
    typedef struct
    {
        unsigned int lengthreceived : 1;
        unsigned int recieving : 1;
        unsigned int recievedDelimeter : 1;
        unsigned int decoderstate : 2;
        unsigned int noCRC : 1;
        unsigned int padding : 2;
    } smp_flags_t;

    /**
     * struct to store the current smpobject
     * */
    typedef struct
    {
        unsigned short bytesToRecieve;
        unsigned char crcHighByte;
        unsigned short crc;
        smp_flags_t flags;
    } smp_struct_t;

    MODULE_API uint16_t SMP_crc16(uint16_t crc, uint16_t c, uint16_t mask);

    // Application functions
    signed char SMP_Init(smp_struct_t *st);
    MODULE_API uint32_t SMP_estimatePacketLength(const byte *buffer, unsigned short length);
    MODULE_API uint32_t SMP_CalculateMinimumSendBufferSize(unsigned short length);
    MODULE_API unsigned int SMP_SendRetIndex(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, unsigned short *messageStartIndex);
    MODULE_API unsigned int SMP_Send(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, byte **messageStartPtr);
    MODULE_API uint16_t SMP_PacketGetLength(const byte *data, uint16_t *headerlength);
    MODULE_API bool SMP_PacketValid(const byte *data, uint16_t packetlength, uint16_t headerlength, uint16_t *crclength);
    MODULE_API smp_decoder_stat SMP_RecieveInByte(byte data, byte* decoded, smp_struct_t *st);
    MODULE_API uint32_t SMP_GetBytesToRecieve(smp_struct_t *st);
    MODULE_API bool SMP_IsRecieving(smp_struct_t *st);
    MODULE_API signed char SMP_getRecieverError(void);

    /**
     * This functions are for internal use only
     * */
    void SMP_ResetDecoderState(smp_struct_t *smp, bool preserveReceivedDelimeter);

#ifdef __cplusplus
}
#endif
