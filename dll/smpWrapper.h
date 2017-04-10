#include "../c/libsmp.h"
#include "main.h"
#include <vector>

#define MESSAGE_BUFFER_SIZE GET_BUFFER_SIZE(65000)

using namespace std;

class SMP
{
public:
    SMP(bool useRS);
    unsigned int estimatePacketLength(const byte* buffer, unsigned short length);
    unsigned char send(const byte *buffer, unsigned short length);
    signed char recieveInBytes(const byte* data, unsigned int length);
    signed char recieveInByte(const byte data);
    byte getBytesToRecieve();
    bool isRecieving();

    size_t MessagesToReceive();
    uint16_t getNextReceivedMessageLength();
    uint8_t getReceivedMessage(message_t* msg);

    size_t getMessagesToSend();
    uint16_t getNextMessageLength();
    uint8_t getMessage(message_t* msg);
private:
    smp_struct_t smp;
    vector<message_t> receiveBuffer;
    vector<message_t> sendBuffer;
    uint8_t messageBuffer[MESSAGE_BUFFER_SIZE];
    fifo_t fifo;
    #ifdef USE_RS_CODE
    ecc_t ecc;
    #endif
};
