#include "smpWrapper.h"

SMP::SMP(smp_struct_t* smp, bool useRS)
{
    this->smp = *smp;
    #ifdef USE_RS_CODE
    if(useRS)
    {
        this.smp.ecc = this.ecc;
    }
    else
        this.smp.ecc = 0;
    #endif
    SMP_Init(&this->smp);
}

unsigned int SMP::estimatePacketLength(const byte* buffer, unsigned short length)
{
    return SMP_estimatePacketLength(buffer,length,&this->smp);
}

unsigned char SMP::send(const byte *buffer, unsigned short length)
{
    return SMP_Send(buffer,length,&this->smp);
}

signed char SMP::recieveInBytes(const byte* data, unsigned int length)
{
    return SMP_RecieveInBytes(data,length,&this->smp);
}

signed char SMP::recieveInByte(const byte data)
{
    return SMP_RecieveInByte(data,&this->smp);
}

byte SMP::getBytesToRecieve()
{
    return SMP_GetBytesToRecieve(&this->smp);
}

bool SMP::isRecieving()
{
    return SMP_IsRecieving(&this->smp) != 0;
}
