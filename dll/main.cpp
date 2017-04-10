#include "main.h"
#include "../c/libsmp.h"
#include <string.h>
#include <inttypes.h>
#include <vector>
#include <new>
#include "smpWrapper.h"
#include <algorithm>

using namespace std;

vector<SMP*> instances;

extern "C" DLL_EXPORT void libsmp_addReceivedBytes(const uint8_t* bytes, uint32_t length, void* obj)
{
	SMP* smp = (SMP*)obj;
	smp->recieveInBytes(bytes,length);
}

extern "C" DLL_EXPORT size_t libsmp_bytesMessagesToReceive(void* obj)
{
	SMP* smp = (SMP*)obj;
    return smp->MessagesToReceive();
}

extern "C" DLL_EXPORT uint16_t libsmp_getNextReceivedMessageLength(void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->getNextReceivedMessageLength();
}

extern "C" DLL_EXPORT uint8_t libsmp_getReceivedMessage(message_t* msg, void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->getReceivedMessage(msg);
}

extern "C" DLL_EXPORT size_t libsmp_getMessagesToSend(void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->getMessagesToSend();
}

extern "C" DLL_EXPORT uint16_t libsmp_getNextMessageLength(void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->getNextMessageLength();
}

extern "C" DLL_EXPORT uint8_t libsmp_getMessage(message_t* msg, void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->getMessage(msg);
}

extern "C" DLL_EXPORT uint32_t libsmp_sendBytes(const uint8_t* bytes, uint32_t length, void* obj)
{
    SMP* smp = (SMP*)obj;
    return smp->send(bytes,length);
}

extern "C" DLL_EXPORT void* libsmp_createNewObject(bool useRS)
{
    SMP* instance = new SMP(useRS);
    instances.push_back(instance);
    return instance;
}

extern "C" DLL_EXPORT void libsmp_deleteObject(void* obj)
{
    vector<SMP*>::iterator it;
    it = find(instances.begin(),instances.end(),(SMP*)obj);
    if(it != instances.end())
    {
        delete *it;
    }
}

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
        // attach to process
        // return FALSE to fail DLL load
        break;

    case DLL_PROCESS_DETACH:
        for(vector<SMP*>::iterator it = instances.begin(); it != instances.end(); ++it)
        {
            delete *it;
        }
        instances.clear();
        break;
    case DLL_THREAD_DETACH:
        // detach from thread
        break;
    }
    return TRUE; // succesful
}
