#include "smpWrapper.h"

static vector<message_t>* currentReceive;
static vector<message_t>* currentSend;

extern "C" signed char frameReady(fifo_t *buffer)
{
    message_t msg;
    if(currentReceive == 0)
    	return -1;
    msg.size = fifo_read_bytes(msg.message,buffer,messageBufferSíze - 1);
    try
    {
    	currentReceive->push_back(msg);
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
    if(currentSend == 0)
    	return 0;
    memcpy(msg.message,buffer,length);
    msg.size = length;
    try
    {
    	currentSend->push_back(msg);
    }
    catch(bad_alloc& ex)
    {
        return 0;
    }

    return length;
}

SMP::SMP(bool useRS)
{
	currentReceive = 0;
	currentSend = 0;
	fifo_init(&this->fifo,this->messageBuffer, messageBufferSíze);
	this->smp.buffer = &this->fifo;
	this->smp.send = sendCallback;
	this->smp.frameReadyCallback = frameReady;
	this->smp.rogueFrameCallback = 0;
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
	unsigned char ret;
	currentSend = &this->sendBuffer;
    ret = SMP_Send(buffer,length,&this->smp);
    currentSend = 0;
    return ret;
}

signed char SMP::recieveInBytes(const byte* data, unsigned int length)
{
	signed char ret;
	currentReceive = &this->receiveBuffer;
    ret = SMP_RecieveInBytes(data,length,&this->smp);
    currentReceive = 0;
    return ret;
}

signed char SMP::recieveInByte(const byte data)
{
	signed char ret;
	currentReceive = &this->receiveBuffer;
    ret = SMP_RecieveInByte(data,&this->smp);
    currentReceive = 0;
    return ret;
}

byte SMP::getBytesToRecieve()
{
    return SMP_GetBytesToRecieve(&this->smp);
}

bool SMP::isRecieving()
{
    return SMP_IsRecieving(&this->smp) != 0;
}
