#include <stdio.h>
#include <stdlib.h>

#include "libsmp.h"

#define MESSAGE_SIZE 50

#define BUFFER_SIZE 60000

unsigned char buffer1[BUFFER_SIZE];
unsigned char buffer2[BUFFER_SIZE];

unsigned char mess[MESSAGE_SIZE];

unsigned char receivedMessage[MESSAGE_SIZE];

fifo_t fifo1;
fifo_t fifo2;

smp_struct_t smp1;
smp_struct_t smp2;

ecc_t ecc1;
ecc_t ecc2;

unsigned char smp1_send(const unsigned char *buffer, unsigned int length)
{
    SMP_RecieveInBytes(buffer,length,&smp2);
    return length;
}

signed char smp1_recieve(fifo_t *buffer)
{
    unsigned char message[MESSAGE_SIZE];

    fifo_read_bytes(message,buffer,MESSAGE_SIZE);

    return 0;
}

unsigned char smp1_intermediate_send(const unsigned char *buffer, unsigned int length)
{
    return SMP_EncodeRS(buffer,length,&smp1);
}

unsigned char smp2_send(const unsigned char *buffer, unsigned int length)
{
    SMP_RecieveInBytes(buffer,length,&smp1);
    return length;
}

signed char smp2_recieve(fifo_t *buffer)
{
    fifo_read_bytes(receivedMessage,buffer,MESSAGE_SIZE);

    return 1;
}

int main()
{
    fifo_init(&fifo1,buffer1,BUFFER_SIZE);
    fifo_init(&fifo2,buffer2,BUFFER_SIZE);

    smp1.buffer = &fifo1;
    smp1.send = smp1_send;
    smp1.frameReadyCallback = smp1_recieve;
    smp1.intermediateSend = smp1_intermediate_send;
    smp1.rogueFrameCallback = 0;
    smp1.ecc = &ecc1;

    smp2.buffer = &fifo2;
    smp2.send = smp2_send;
    smp2.frameReadyCallback = smp2_recieve;
    smp2.rogueFrameCallback = 0;
    smp2.ecc = &ecc2;

    SMP_Init(&smp1);
    SMP_Init(&smp2);

    int i;
    for(i = 0; i < MESSAGE_SIZE; i++)
    {
        mess[i] = rand() & 0xFF;
    }

    mess[2] = 255;
    mess[10] = 255;

    SMP_Send(mess,MESSAGE_SIZE,0,&smp1);



    int d;
    char ans = 1;
    for(d = 0; d < i; d++)
    {
        if(mess[d] != receivedMessage[d])
        {
            ans = 0;
            break;
        }
    }

    return 0;
}
