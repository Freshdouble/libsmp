/*****************************************************************************************************

 Autor: Peter Kremsner
 Date: 12.9.2014

 ******************************************************************************************************/

#ifndef _SMP_H__
#define _SMP_H__

#define FRAMESTART 0xFF									// The framestartdelimeter														// if outputbuffering is disabled you can only read the incoming date with the FrameReadyCallback
#define CRC_POLYNOM 0xA001

#include "../libfifo/c/libfifo.h"

//Typedefinitions
#ifndef _INTTYPES_H_
typedef unsigned char uint8_t;
typedef signed char int8_t;
#endif
typedef uint8_t byte;

//Callbacks
//When the callbackfunction returns a negative Integer, its treated as error code.
//When the length and the bufferpointer is both zero, then this function should return the error Code
typedef signed char (*SMP_Frame_Ready)(fifo_t* data); //FrameReadyCallback: Length is the ammount of bytes in the recieveBuffer

/**********
Sends arbitrary number of bytes over interface.
Returns the number of bytes that were sent
*/
typedef unsigned char (*SMP_send_function)(unsigned char * buffer, unsigned int length);

typedef struct
{
	unsigned short bytesToRecieve;
	fifo_t* buffer; //Buffersize must match 1 Byte (sizeof(uint8_t))

	SMP_send_function send;
	SMP_Frame_Ready frameReadyCallback;
	SMP_Frame_Ready rogueFrameCallback;

	unsigned char crcHighByte;
	unsigned short crc;
	struct
	{
		unsigned int recieving :1;
		unsigned int recievedDelimeter :1;
		unsigned int error :4;
		unsigned int status :2;
	} flags;
} smp_struct_t;

//Application functions
signed char SMP_Init(smp_struct_t* st, fifo_t* buffer, SMP_send_function send, SMP_Frame_Ready frameReadyCallback, SMP_Frame_Ready rogueFrameCallback);
unsigned char SMP_Send(byte *buffer, unsigned short length,smp_struct_t *st);
/**
 * When one recievefunction returns an error, the error code should be parsed.
 * To avoid a buffer overflow it is recomended that no data is sent to the reciever until the error is cleared.
 * If an error is returned it is not sure that the data recieved the reciever.
 */
signed char SMP_RecieveInBytes(byte* data, unsigned int length, smp_struct_t* st);
signed char SMP_RecieveInByte(byte data, smp_struct_t* st);
byte SMP_GetBytesToRecieve(smp_struct_t* st);
byte SMP_IsRecieving(smp_struct_t* st);
signed char SMP_getRecieverError(void);

#endif // _SMP_H__
