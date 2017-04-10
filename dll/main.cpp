#include "main.h"
#include "../c/libsmp.h"
#include <string.h>
#include <inttypes.h>
#include <vector>
#include <new>
#include "smpWrapper.h"

using namespace std;

extern "C" DLL_EXPORT void libsmp_addReceivedBytes(const uint8_t* bytes, uint32_t length, void* obj)
{
	SMP* smp = obj;
	smp->recieveInBytes(data,legnth);
}

extern "C" DLL_EXPORT size_t libsmp_bytesMessagesToReceive(void* obj)
{
	SMP* smp = obj;
    return receiveBuffer.size();
}

extern "C" DLL_EXPORT uint16_t libsmp_getNextReceivedMessageLength(void* obj)
{
    if(receiveBuffer.empty())
        return 0;
    message_t msg = receiveBuffer.back();
    return msg.size;
}

extern "C" DLL_EXPORT uint8_t libsmp_getReceivedMessage(message_t* msg, void* obj)
{
    if(!receiveBuffer.empty())
    {
        *msg = receiveBuffer.back();
        receiveBuffer.pop_back();
        return 1;
    }
    else
        msg->size = 0;
    return 0;
}

extern "C" DLL_EXPORT size_t libsmp_getMessagesToSend(void* obj)
{
    return sendBuffer.size();
}

extern "C" DLL_EXPORT uint16_t libsmp_getNextMessageLength(void* obj)
{
    if(sendBuffer.empty())
        return 0;
    message_t msg = sendBuffer.back();
    return msg.size;
}

extern "C" DLL_EXPORT uint8_t libsmp_getMessage(message_t* msg, void* obj)
{
    if(!sendBuffer.empty())
    {
        *msg = sendBuffer.back();
        sendBuffer.pop_back();
        return 1;
    }
    else
        msg->size = 0;
    return 0;
}

extern "C" DLL_EXPORT uint32_t libsmp_sendBytes(const uint8_t* bytes, uint32_t length, void* obj)
{
    return SMP_Send(bytes,length,&smp);
}

extern "C" DLL_EXPORT void libsmp_useRS(BOOL rs)
{
    fifo_init(&fifo,messageBuffer,(uint16_t)sizeof(messageBuffer));
    smp.buffer = &fifo;
    smp.frameReadyCallback = frameReady;
    smp.send = sendCallback;
    smp.rogueFrameCallback = 0;
    #ifdef USE_RS_CODE
    if(rs)
    {
        smp.ecc = &ecc;
    }
    else
    {
        smp.ecc = 0;
    }
    #endif
    SMP_Init(&smp);
}

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        libsmp_useRS(FALSE);
        receiveBuffer.clear();
        sendBuffer.clear();
        break;
    case DLL_THREAD_ATTACH:
        // attach to process
        // return FALSE to fail DLL load
        break;

    case DLL_PROCESS_DETACH:
        receiveBuffer.clear();
        sendBuffer.clear();
        break;
    case DLL_THREAD_DETACH:
        // detach from thread
        break;
    }
    return TRUE; // succesful
}
