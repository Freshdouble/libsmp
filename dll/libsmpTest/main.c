#include <stdio.h>
#include <stdlib.h>
#include "../main.h"
#include <inttypes.h>

int main()
{
    const uint8_t message[] = {22,10,1,240,255,0,1,1,2,2,2,2,2,2};
    libsmp_useRS(1);
    libsmp_sendBytes(message,sizeof(message));
    message_t msg;
    libsmp_getMessage(&msg);
    libsmp_addReceivedBytes(msg.message,msg.size);
    libsmp_getReceivedMessage(&msg);

    return 0;
}
