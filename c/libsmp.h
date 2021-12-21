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
#define MINIMUM_SMP_BUFFERLENGTH(maxmessagelength) (2 * (length + 2) + 5)

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAMESTART 0xFF    // The framestartdelimeter
#define CRC_POLYNOM 0xA001 // CRC Generatorpolynom

    typedef uint8_t byte;

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
     * struct to hold the settings of the smp
     * */
    typedef struct
    {
        struct
        {
            uint8_t *buffer;
            uint32_t maxlength;
        } buffer;

        SMP_Frame_Ready frameReadyCallback;
        SMP_Frame_Ready rogueFrameCallback;
    } smp_settings_t;

    /**
     * struct to store the current smpobject
     * */
    typedef struct
    {
        unsigned short bytesToRecieve;
        unsigned char crcHighByte;
        unsigned short crc;
        smp_flags_t flags;
        uint8_t *writePtr;
    } smp_status_t;

    typedef struct
    {
        smp_settings_t settings;
        smp_status_t status;
    } smp_struct_t;

    /***********************************************************/
    /**
     * This functions only exist when the library was compiled with the CREATE_ALLOC_LAYER option
     * This functions are not compiled for the cross compile arm static library scince this
     * build is intended for µC use
     */
    MODULE_API smp_struct_t *SMP_BuildObject(uint32_t bufferlength, SMP_Frame_Ready frameReadyCallback, SMP_Frame_Ready rogueFrameCallback);
    MODULE_API void SMP_DestroyObject(smp_struct_t *st);
    /**********************************************************/

    // Application functions
    signed char SMP_Init(smp_struct_t *st, smp_settings_t *settings);
    MODULE_API uint32_t SMP_estimatePacketLength(const byte *buffer, unsigned short length);
    MODULE_API uint32_t SMP_CalculateMinimumSendBufferSize(unsigned short length);
    MODULE_API unsigned int SMP_SendRetIndex(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, unsigned short *messageStartIndex);
    MODULE_API unsigned int SMP_Send(const byte *buffer, unsigned short length, byte *messageBuffer, unsigned short bufferLength, byte **messageStartPtr);

    MODULE_API uint16_t SMP_PacketGetLength(const byte *data, uint16_t *headerlength);
    MODULE_API bool SMP_PacketValid(const byte *data, uint16_t packetlength, uint16_t headerlength, uint16_t *crclength);

    /**
     * When one recievefunction returns an error, the error code should be parsed.
     * To avoid a buffer overflow it is recomended that no data is sent to the reciever until the error is cleared.
     * If an error is returned it is not sure that the data recieved the reciever.
     */
    MODULE_API signed char SMP_RecieveInBytes(const byte *data, uint32_t length, smp_struct_t *st);
    MODULE_API signed char SMP_RecieveInByte(const byte data, smp_struct_t *st);
    MODULE_API uint32_t SMP_GetBytesToRecieve(smp_struct_t *st);
    MODULE_API bool SMP_IsRecieving(smp_struct_t *st);
    MODULE_API signed char SMP_getRecieverError(void);

    MODULE_API static inline uint8_t *SMP_GetBuffer(smp_struct_t *st)
    {
        return st->settings.buffer.buffer;
    }

    MODULE_API static inline uint16_t SMP_LastReceivedMessageLength(smp_struct_t *st)
    {
        return st->status.writePtr - st->settings.buffer.buffer;
    }

    /**
     * This functions are for internal use only
     * */
    void SMP_ResetDecoderState(smp_struct_t *smp, bool preserveReceivedDelimeter);

#ifdef __cplusplus
}
#endif
