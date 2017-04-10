#include "../c/libsmp.h"

class SMP
{
public:
    SMP(smp_struct_t* smp, bool useRS);
    unsigned int estimatePacketLength(const byte* buffer, unsigned short length);
    unsigned char send(const byte *buffer, unsigned short length);
    signed char recieveInBytes(const byte* data, unsigned int length);
    signed char recieveInByte(const byte data);
    byte getBytesToRecieve();
    bool isRecieving();
private:
    smp_struct_t smp;
    #ifdef USE_RS_CODE
    ecc_t ecc;
    #endif
};
