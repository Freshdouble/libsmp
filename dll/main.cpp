#include "main.h"
#include "../c/libsmp.h"
#include <string.h>
#include <inttypes.h>
#include <vector>
#include <new>

using namespace std;

static smp_struct_t smp;
static uint8_t messageBuffer[GET_BUFFER_SIZE(65536)];
static fifo_t fifo;
#ifdef USE_RS_CODE
static ecc_t ecc;
#endif // USE_RS_CODE

static vector<message_t> receiveBuffer;
static vector<message_t> sendBuffer;

extern "C" signed char frameReady(fifo_t *buffer)
{
    uint8_t data;
    uint32_t i = 0;
    message_t msg;
    while(fifo_read_byte(&data,buffer))
    {
        msg.message[i] = data;
        i++;
    }
    msg.size = i;
    try
    {
        receiveBuffer.push_back(msg);
    }
    catch(bad_alloc& ex)
    {
        return -1;
    }

    return 0;
}

extern "C" unsigned char sendCallback(unsigned char * buffer, unsigned int length)
{
    message_t msg;
    memcpy(msg.message,buffer,length);
    msg.size = length;
    try
    {
        sendBuffer.push_back(msg);
    }
    catch(bad_alloc& ex)
    {
        return 0;
    }

    return length;
}

extern "C" DLL_EXPORT void libsmp_addReceivedBytes(const uint8_t* bytes, uint32_t length)
{
    SMP_RecieveInBytes(bytes,length,&smp);
}

extern "C" DLL_EXPORT size_t libsmp_bytesMessagesToReceive()
{
    return receiveBuffer.size();
}

extern "C" DLL_EXPORT uint8_t libsmp_getReceivedMessage(message_t* msg)
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

extern "C" DLL_EXPORT size_t libsmp_getMessagesToSend()
{
    return sendBuffer.size();
}

extern "C" DLL_EXPORT uint16_t libsmp_getNextMessageLength()
{
    if(sendBuffer.empty())
        return 0;
    message_t msg = sendBuffer.back();
    return msg.size;
}

extern "C" DLL_EXPORT uint8_t libsmp_getMessage(message_t* msg)
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

extern "C" DLL_EXPORT uint32_t libsmp_sendBytes(const uint8_t* bytes, uint32_t length)
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
    if(rs)
    {
        smp.ecc = &ecc;
    }
    else
    {
        smp.ecc = 0;
    }
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
