#ifndef __MAIN_H__
#define __MAIN_H__

#include <windows.h>

/*  To use this exported function of dll, include this header
 *  in your project.
 */

#include <inttypes.h>

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t message[65536];
    uint32_t size;
}message_t;

DLL_EXPORT void libsmp_addReceivedBytes(const uint8_t* bytes, uint32_t length);
DLL_EXPORT size_t libsmp_bytesMessagesToReceive();
DLL_EXPORT uint16_t libsmp_getNextReceivedMessageLength();
DLL_EXPORT uint8_t libsmp_getReceivedMessage(message_t* msg);
DLL_EXPORT size_t libsmp_getMessagesToSend();
DLL_EXPORT uint16_t libsmp_getNextMessageLength();
DLL_EXPORT uint8_t libsmp_getMessage(message_t* msg);
DLL_EXPORT uint32_t libsmp_sendBytes(const uint8_t* bytes, uint32_t length);
DLL_EXPORT void libsmp_useRS(BOOL rs);

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H__