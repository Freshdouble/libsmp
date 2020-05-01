#include "../libsmp.h"
#include "../fecmp/libfecmp.h"
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef STABILITY_RUNS
#define STABILITY_RUNS 1000000
#endif

static bool SequenceEqual(uint8_t *seq1, uint8_t *seq2, uint32_t length)
{
    while (length > 0)
    {
        length--;
        if (*seq1 != *seq2)
        {
            return false;
        }
        seq1++;
        seq2++;
    }
    return true;
}

static void printPacket(uint8_t *data, uint32_t length)
{
    for (int i = 0; i < length; i++)
    {
        if (i > 0)
        {
            printf(",");
        }
        else
        {
            printf("\t");
        }
        
        printf("0x%02X", data[i]);
    }
    printf("\n");
}

uint8_t databuffer[1024];
smp_struct_t smp;

uint8_t latestReceived[1024];
uint32_t latestReceivedLength;

signed char datareceived(uint8_t *data, uint32_t length)
{
    memcpy(latestReceived, data, length);
    latestReceivedLength = length;
    printf("Received packet\n");
    return 1;
}

int main(void)
{
    uint8_t rawsendmessage[1024];
    uint8_t sendbuffer[2048];
    uint32_t i;
    uint32_t toSend;
    uint8_t *messageStart;
    bool haderrors = false;
    smp_settings_t settings;
    memset(&settings, 0, sizeof(smp_settings_t));
    settings.buffer.buffer = databuffer;
    settings.buffer.maxlength = sizeof(databuffer);
    settings.frameReadyCallback = datareceived;
    SMP_Init(&smp, &settings);
    srand(time(0));

    //Sending 256 byte
    haderrors = false;
    latestReceivedLength = 0;
    memset(latestReceived, 0, sizeof(latestReceived));
    printf("Testing 256 byte\n");
    for (i = 0; i < 256; i++)
    {
        rawsendmessage[i] = rand() & 0xFF;
    }
    toSend = SMP_Send(rawsendmessage, 256, sendbuffer, sizeof(sendbuffer), &messageStart);
    if (toSend)
    {
        printf("Sending %ld data over smp. Raw size: %d\n", toSend, i);
        SMP_RecieveInBytes(messageStart, toSend, &smp);
        if (latestReceivedLength != 256)
        {
            printf("Expected %ld but received %ld", 256, latestReceivedLength);
            return -1;
        }
        if (!SequenceEqual(latestReceived, rawsendmessage, latestReceivedLength))
        {
            printf("256 bytes were not transmitted successfully\n");
            printf("Original: ");
            printPacket(rawsendmessage, 256);
            printf("Received: ");
            printPacket(latestReceived, latestReceivedLength);
            return -1;
        }
        printf("PASSED!\n");
    }
    else
    {
        printf("Couldn't send 256 bytes of data");
        return -1;
    }

    //Sending 256 byte random error
    for (int d = 0; d < 10; d++)
    {
        haderrors = false;
        latestReceivedLength = 0;
        memset(latestReceived, 0, sizeof(latestReceived));
        printf("Testing 256 byte with random error. Iterration Number: %d\n", d + 1);
        for (i = 0; i < 256; i++)
        {
            rawsendmessage[i] = rand() & 0xFF;
        }
        toSend = SMP_Send(rawsendmessage, 256, sendbuffer, sizeof(sendbuffer), &messageStart);
        if (toSend)
        {
            printf("Sending %ld data over smp. Raw size: %d\n", toSend, i);
            for (i = 0; i < toSend; i++)
            {
                int random = (uint8_t)(rand() & 0xff);
                if (random > 140 && random < 150)
                {
                    uint8_t old = messageStart[i];
                    messageStart[i] = rand() & 0xff;
                    if (messageStart[i] != old)
                        haderrors = true;
                    printf("\tChanging position %ld from %d to %d\n", i, old, messageStart[i]);
                }
            }
            if (!haderrors)
            {
                printf("No random errors, forcing error");
                latestReceived[3] = 0xff;
            }
            SMP_RecieveInBytes(messageStart, toSend, &smp);
            if (haderrors && latestReceivedLength != 0)
            {
                printf("Message passed checks but had errors\n");
                printf("Rogue packet: ");
                printPacket(messageStart, toSend);
                return -1;
            }
            printf("PASSED!\n");
        }
        else
        {
            printf("Couldn't send 256 bytes of data");
            return -1;
        }
    }
#if STABILITY_RUNS > 0
    //Data for metrics
    uint64_t rawdatacount = 0;
    uint64_t transmitteddatacount = 0;
    uint64_t addederrors = 0;
    uint64_t crcerrors = 0;
    uint64_t correctmessagecount = 0;
    uint64_t roguemessagecount = 0;

#define USELIBFEC
#ifdef USELIBFEC
    libfecmp_t fec;
    uint8_t fecbuffer[1024*BLOCKSIZE];
    libfecmp_settings_t fecsettings;
    fecsettings.data = fecbuffer;
    fecsettings.bufferlength = sizeof(fecbuffer);
    fec_Init(&fec, &fecsettings);
#endif

    //Sending random byte length with random errors
    for (int d = 0; d < STABILITY_RUNS; d++)
    {
        haderrors = false;
        latestReceivedLength = 0;
        memset(latestReceived, 0, sizeof(latestReceived));
        uint32_t testdatalength = rand() % 100 + 10;
        printf("Testing %ld byte with random error. Iterration Number: %d\n", testdatalength, d + 1);
        for (i = 0; i < testdatalength; i++)
        {
            rawsendmessage[i] = rand() & 0xFF;
        }
        rawdatacount += testdatalength;
#ifdef USELIBFEC
        //Test packet with fec
        memset(sendbuffer, 0, sizeof(sendbuffer));
        toSend = fec_encode(rawsendmessage, testdatalength, sendbuffer, sizeof(sendbuffer), &messageStart, &fec);
        if(toSend > 0)
        {
            printPacket(messageStart, toSend);
        }
        return 0;
#endif
        toSend = SMP_Send(rawsendmessage, testdatalength, sendbuffer, sizeof(sendbuffer), &messageStart);
        transmitteddatacount += toSend;
        if (toSend)
        {
            printf("Sending %ld data over smp. Raw size: %d\n", toSend, i);
            for (i = 0; i < toSend; i++)
            {
                uint32_t random = (uint8_t)(rand() & 0xff);
                if (random < 5)
                {
                    uint8_t old = messageStart[i];
                    messageStart[i] = rand() & 0xff;
                    if (messageStart[i] != old)
                    {
                        haderrors = true;
                        addederrors++;
                    }
                    printf("\tChanging position %ld from %d to %d\n", i, old, messageStart[i]);
                }
            }
            SMP_RecieveInBytes(messageStart, toSend, &smp);
            if (haderrors)
            {
                roguemessagecount++;
                if (latestReceivedLength != 0)
                {
                    if(SequenceEqual(rawsendmessage, latestReceived, latestReceivedLength))
                    {
                        printf("This shouldn't happen!\n");
                    }
                    //We have not detected the errors
                    printf("Message passed checks but had errors\n");
                    printf("Rogue packet: ");
                    printPacket(messageStart, toSend);
                    crcerrors++;
                    #ifdef QUIT_ON_CRC_ERROR
                    return -1;
                    #endif
                }
            }
            else
            {
                correctmessagecount++;
                //We didn't have errors check the received data
                if(latestReceivedLength != testdatalength)
                {
                }
                if (!SequenceEqual(rawsendmessage, latestReceived, latestReceivedLength))
                {
                    //We didn't pass the check
                    printf("%ld bytes were not transmitted successfully\n", testdatalength);
                    printf("Original: ");
                    printPacket(rawsendmessage, testdatalength);
                    printf("Sent data: ");
                    printPacket(messageStart, toSend);
                    printf("Received: ");
                    printPacket(latestReceived, latestReceivedLength);
                    return -1;
                }
            }

            printf("PASSED!\n");
        }
        else
        {
            printf("Couldn't send %ld bytes of data\n", testdatalength);
            printPacket(rawsendmessage, testdatalength);
            return -1;
        }
    }

    printf("Stability tests passed with metrics:\n");
    printf("\tTotal message count: %ld\n", STABILITY_RUNS);
    printf("\tAverage bytes per message: %.2f\n",(double)transmitteddatacount/(double)STABILITY_RUNS);
    printf("\n");
    printf("\tRaw bytes sent: %ld\n", rawdatacount);
    printf("\tProcessed bytes sent: %ld\n", transmitteddatacount);
    printf("\tOverhead: %.2f %%\n", ((double)transmitteddatacount - (double)rawdatacount) * 100.0 / ((double)transmitteddatacount + (double)rawdatacount));
    printf("\tTotal error bytes added %ld\n", addederrors);
    printf("\tTotal messages with errors %ld\n", roguemessagecount);
    printf("\tTotal messages without errors %ld\n", correctmessagecount);
    printf("\tByte error rate: %.2f %%\n", (double)addederrors * 100.0 / (double)transmitteddatacount);
    printf("\tMessage error rate: %.2f %%",(double)roguemessagecount * 100.0/((double)roguemessagecount + (double)correctmessagecount));
    printf("\tTotal CRC Errors: %ld\n",crcerrors);
    printf("\tCRC Errorrate: %.4f %%\n",(double)crcerrors * 100.0/(double)STABILITY_RUNS);
#endif
    return 0;
}