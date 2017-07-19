/*****************************************************************************************************
 
 Autor: Peter Kremsner
 Date: 12.9.2014
 
 ******************************************************************************************************/

#ifndef _SMP_H__
#define _SMP_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define FRAMESTART 0xFF	    // The framestartdelimeter
#define CRC_POLYNOM 0xA001 //CRC Generatorpolynom
    //Add USE_RS_CODE preprocessor constant to enable RS_Encoding

#ifdef USE_RS_CODE
#define Blocksize 16UL //define size of one Block, must be smaler than 255
#if Blocksize > 255
#error "Maximum Blocksize exeded, use a value smaler than 255"
#endif // Blocksize
#define BlockData (Blocksize - NPAR)
#endif // USE_RS_CODE

#include "libfifo.h"

#ifdef USE_RS_CODE
#include "ecc.h"
#endif // USE_RS_CODE

    //Typedefinitions
#ifndef _INTTYPES_H_
    typedef unsigned char uint8_t;
    typedef signed char int8_t;
#endif

    typedef uint8_t byte;

#define SMP_SEND_BUFFER_LENGTH(messageLength) (2*(messageLength + 2) + 5)

    //Callbacks
    //When the callbackfunction returns a negative Integer, its treated as error code.
    //When the length and the bufferpointer is both zero, then this function should return the error Code
    typedef signed char (*SMP_Frame_Ready)(fifo_t* data); //FrameReadyCallback: Length is the ammount of bytes in the recieveBuffer

    /**********
    Sends arbitrary number of bytes over interface.
    Returns the number of bytes that were sent
    */
    typedef unsigned int (*SMP_send_function)(unsigned char * buffer, unsigned int length);

    typedef struct
    {
        unsigned short bytesToRecieve;
        fifo_t* buffer; //Buffersize must match 1 Byte (sizeof(uint8_t))
        SMP_Frame_Ready frameReadyCallback;
        SMP_Frame_Ready rogueFrameCallback;

        unsigned char crcHighByte;
        unsigned short crc;
        struct
        {
unsigned int recieving :
            1;
unsigned int recievedDelimeter :
            1;
unsigned int status :
            2;
unsigned int noCRC :
            1;
        }
        flags;

#ifdef USE_RS_CODE

        unsigned char rsBuffer[Blocksize];
        unsigned short rsPtr;
        ecc_t* ecc;
#endif // USE_RS_CODE

    }
    smp_struct_t;

    //Application functions
    signed char SMP_Init(smp_struct_t* st);
    unsigned int SMP_estimatePacketLength(const byte* buffer, unsigned short length);
    unsigned int SMP_Send(const byte *buffer, unsigned short length, byte* messageBuffer, unsigned short bufferLength, byte** messageStartPtr);
    /**
     * When one recievefunction returns an error, the error code should be parsed.
     * To avoid a buffer overflow it is recomended that no data is sent to the reciever until the error is cleared.
     * If an error is returned it is not sure that the data recieved the reciever.
     */
    signed char SMP_RecieveInBytes(const byte* data, unsigned int length, smp_struct_t* st);
    signed char SMP_RecieveInByte(const byte data, smp_struct_t* st);
    byte SMP_GetBytesToRecieve(smp_struct_t* st);
    byte SMP_IsRecieving(smp_struct_t* st);
    signed char SMP_getRecieverError(void);

#ifdef __cplusplus
}
#endif

#endif // _SMP_H__
