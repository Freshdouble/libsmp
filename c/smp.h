/*****************************************************************************************************

Autor: Peter Kremsner
Date: 12.9.2014

******************************************************************************************************/

#ifndef _SMP_H__
#define _SMP_H__

#define FRAMESTART 0xFF									// The framestartdelimeter														// if outputbuffering is disabled you can only read the incoming date with the FrameReadyCallback
#define MAX_PAYLOAD 30			// The maximum size of the payload in one Frame
#define MAX_FRAMESIZE MAX_PAYLOAD + 4
#define CRC_POLYNOM 0xA001

#include "../Fifo/c/stc_fifo.h"

//Typedefinitions
#ifndef _INTTYPES_H_
typedef unsigned char uint8_t;
typedef signed char int8_t;
#endif
typedef uint8_t byte;

struct smp_buffer
{
    unsigned char Message[MAX_PAYLOAD];
    unsigned char ptr;
};

typedef union
{
    struct
    {
        unsigned int bufferFull:1;
    }flags;
    byte errorCode;
}smp_error_t;

typedef struct
{
    unsigned char bytesToRecieve;
    struct smp_buffer buffer; //Payload
	unsigned char crcHighByte;
	unsigned short crc;
    struct
    {
	    unsigned int recieving:1;
	    unsigned int recievedDelimeter:1;
	    unsigned int error:4;
	    unsigned int status:2;
    } flags;
}smp_struct_t;

//Application functions
void SMP_Init(smp_struct_t* st);
unsigned char SMP_Send(byte *buffer, byte length, byte* smpBuffer, byte smpBuffersize);
/**
 * When one recievefunction returns an error, the error code should be parsed.
 * To avoid a buffer overflow it is recomended that no data is sent to the reciever until the error is cleared.
 * If an error is returned it is not sure that the data recieved the reciever.
 */
smp_error_t SMP_RecieveInBytes(byte* data, byte length, smp_struct_t* st);
smp_error_t SMP_RecieveInByte(byte data, smp_struct_t* st);
byte SMP_GetBytesToRecieve(smp_struct_t* st);
byte SMP_IsRecieving(smp_struct_t* st);
smp_error_t SMP_getRecieverError(void);

//Callbacks
//When the callbackfunction returns a negative Integer, its treated as error code.
//When the length and the bufferpointer is both zero, then this function should return the error Code
typedef smp_error_t (*SMPframeReady)(byte length, byte* buffer); //FrameReadyCallback: Length is the ammount of bytes in the recieveBuffer
void SMP_RegisterFrameReadyCallback(SMPframeReady func);
void SMP_UnregisterFrameReadyCallback(void);

#endif // _SMP_H__
