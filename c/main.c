#include <stdio.h>
#include <stdlib.h>

#include "libsmp.h"

#define MESSAGE_SIZE 25000

#define BUFFER_SIZE 60000

unsigned char buffer1[BUFFER_SIZE];
unsigned char buffer2[BUFFER_SIZE];

unsigned char mess[MESSAGE_SIZE];

fifo_t fifo1;
fifo_t fifo2;

smp_struct_t smp1;
smp_struct_t smp2;

unsigned char smp1_send(unsigned char *buffer, unsigned int length)
{
    SMP_RecieveInBytes(buffer,length,&smp2);
    return length;
}

signed char smp1_recieve(fifo_t *buffer)
{
    unsigned char message[MESSAGE_SIZE];
    int i;

    i = fifo_read(message,MESSAGE_SIZE,buffer);

    return 0;
}

unsigned char smp2_send(unsigned char *buffer, unsigned int length)
{
    SMP_RecieveInBytes(buffer,length,&smp1);
    return length;
}

signed char smp2_recieve(fifo_t *buffer)
{
    unsigned char message[MESSAGE_SIZE];
    int ans = 1;
    int i;
    int d;

    i = fifo_read(message,MESSAGE_SIZE,buffer);

    for(d = 0; d < i; d++)
    {
        if(message[d] != mess[d])
        {
            ans = 0;
            break;
        }
    }

    return 0;
}

int main()
{
    fifo_init(&fifo1,buffer1,BUFFER_SIZE,1);
    fifo_init(&fifo2,buffer2,BUFFER_SIZE,1);

    SMP_Init(&smp1,&fifo1,smp1_send,smp1_recieve,0);
    SMP_Init(&smp2,&fifo2,smp2_send,smp2_recieve,0);

    int i;
    for(i = 0; i < MESSAGE_SIZE; i++)
    {
        mess[i] = rand() & 0xFF;
    }

    SMP_Send(mess,MESSAGE_SIZE,&smp1);

    return 0;
}
