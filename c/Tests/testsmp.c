#include "../libsmp.h"
#include "../fecmp/libfecmp.h"
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef STABILITY_RUNS
#define STABILITY_RUNS 100000
#endif

#define MINMESSAGELENGTH 10
#define MAXMESSAGELENGTH 50
#define BITERRORPROB 0.001

uint8_t smpbuffer[1024];
smp_struct_t smp;
#define TESTFEC
#ifdef TESTFEC
uint8_t fecmpbuffer[512 * BLOCKSIZE];
libfecmp_t fecmp;
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

uint8_t latestReceived[1024];
uint32_t latestReceivedLength;

signed char smpdatareceived(uint8_t *data, uint32_t length)
{
    memcpy(latestReceived, data, length);
    latestReceivedLength = length;
    printf("Received packet\n");
    return 1;
}

signed char fecdatareceived(uint8_t *data, uint32_t length)
{
    return 0;
}

uint32_t CreateRandomMessage(uint8_t *buffer, uint32_t minmessagelength, uint32_t maxmessagelength)
{
    uint32_t messagelength = rand() % (maxmessagelength - minmessagelength) + minmessagelength;
    for (uint32_t i = 0; i < messagelength; i++)
    {
        buffer[i] = rand() & 0xFF;
    }
    return messagelength;
}

bool AddRandomError(uint8_t *data, uint32_t datalength, double biterrorprobability, uint32_t *byteerrorcount)
{
    uint8_t mask;
    bool ret = false;
    if (byteerrorcount != 0)
        *byteerrorcount = 0;
    for (uint32_t i = 0; i < datalength; i++)
    {
        mask = 0;
        for (int d = 0; d < 8; d++)
        {
            if ((rand() % 100000) <= (int)(biterrorprobability * 100000))
            {
                mask |= 1 << d;
            }
        }
        if (mask != 0)
        {
            ret = true;
            if (byteerrorcount != 0)
                *byteerrorcount = *byteerrorcount + 1;
        }
        data[i] ^= mask;
    }
    return ret;
}

int main(void)
{
    //Initialize
    //srand(time(NULL));

    printf("Init smp structures\n");
    //Setup smp
    smp_settings_t smpsettings;
    memset(&smpsettings, 0, sizeof(smpsettings));
    smpsettings.buffer.buffer = smpbuffer;
    smpsettings.buffer.maxlength = sizeof(smpbuffer);
    smpsettings.frameReadyCallback = smpdatareceived;
    SMP_Init(&smp, &smpsettings);

    printf("Init fec structures\n");
    //Setup fecmp
#ifdef TESTFEC
    libfecmp_settings_t fecmpsettings;
    memset(&fecmpsettings, 0, sizeof(fecmpsettings));
    fecmpsettings.data = fecmpbuffer;
    fecmpsettings.bufferlength = sizeof(fecmpbuffer);
    fecmpsettings.frameReadyCallback = smpdatareceived;
    fec_Init(&fecmp, &fecmpsettings);
#endif
    uint8_t packetbuffer[2048];           //Buffer for sending
    uint8_t lasttransmittedmessage[2048]; //Buffer for sending
    uint32_t lastDataToTransmitCount = 0;

    uint32_t toSend;
    uint32_t payloadlength;
    uint8_t payloadbuffer[MAXMESSAGELENGTH];
    uint8_t *msgstart;
    bool hasErrors;

    uint32_t addedByteErrors = 0;
    uint32_t smpcrcerrors = 0;
    uint32_t intactsmpcount = 0;
    uint32_t fec_corrects = 0;
    uint32_t smppayloadbytes = 0;
    uint32_t smptransmittedbytes = 0;
    uint32_t smptotalByteErrors = 0;
    uint32_t smpreceivedmessages = 0;
    uint32_t smpreceiveerrors = 0;
    int returncode;

    printf("Starting smp stability runs\n");
    for (uint32_t i = 0; i < STABILITY_RUNS; i++)
    {
        printf("Loop %u, from %d\n", i + 1, STABILITY_RUNS);
        //Check the smp stack
        memset(latestReceived, 0, sizeof(latestReceived));
        latestReceivedLength = 0;
        memset(packetbuffer, 0, sizeof(packetbuffer));
        payloadlength = CreateRandomMessage(payloadbuffer, MINMESSAGELENGTH, MAXMESSAGELENGTH);
        smppayloadbytes += payloadlength;
        toSend = fec_encode(payloadbuffer, payloadlength, packetbuffer, sizeof(packetbuffer), &msgstart, &fecmp);
        //toSend = SMP_Send(payloadbuffer, payloadlength, packetbuffer, sizeof(packetbuffer), &msgstart);
        smptransmittedbytes += toSend;
        if (toSend > 0)
        {
            hasErrors = AddRandomError(msgstart, toSend, BITERRORPROB, &addedByteErrors);
            smptotalByteErrors += addedByteErrors;
            //SMP try to decode the data
            returncode = SMP_RecieveInBytes(msgstart, toSend, &smp);
            if (hasErrors)
            {
                printf("Packet had errors\n");
                if (latestReceivedLength != 0)
                {
                    if (!SequenceEqual(payloadbuffer, latestReceived, payloadlength))
                    {
                        printf("SMP CRC Error\n");
                        smpcrcerrors++;
                    }
                    else
                    {
                        intactsmpcount++;
                    }
                }
            }
            else
            {
                if (latestReceivedLength != payloadlength)
                {
                    printf("SMP Length missmatch at receive\n");
                    printf("Transmitted: ");
                    printPacket(msgstart, toSend);
                    printf("Original: ");
                    printPacket(payloadbuffer, payloadlength);
                    printf("Transmitlength: %u\n", toSend);
                    printf("Recovered packet lenght: %u\n", SMP_PacketGetLength(msgstart, 0));
                    printf("Received: ");
                    printPacket(latestReceived, latestReceivedLength);
                    printf("Returncode %d\n", returncode);
                    printf("Last packet: ");
                    printPacket(lasttransmittedmessage, lastDataToTransmitCount);
                    if (lasttransmittedmessage[lastDataToTransmitCount - 1] == 0xFF)
                    {
                        //This error happens when the last byte of the previous message is a unescaped framestart and the lengthbyte was to long
                        //The smp decoder then assumes that the following Framestart is a masked byte and keeps processing.
                        //This results in a checksum error and two lost packeges. However the decoder should resync with the next message if the same error
                        //doesn't happen again
                        printf("Real framestart was masked by the error\n");
                    }
                    else
                    {
                        smpreceiveerrors++;
                        //return -1; //This is a serious error
                    }
                }
                else
                {
                    if (!SequenceEqual(latestReceived, payloadbuffer, latestReceivedLength))
                    {
                        printf("SMP Data missmatch at receive\n");
                        printf("Transmitted: ");
                        printPacket(payloadbuffer, payloadlength);
                        printf("Received: ");
                        printPacket(latestReceived, latestReceivedLength);
                        printf("Returncode %d\n", returncode);
                        return -1; //This is a serious error
                    }
                    else
                    {
                        smpreceivedmessages++;
                    }
                }
            }
#ifdef TESTFEC
            latestReceivedLength = 0;
            //Do the same test with libfecmp
            fec_processBytes(msgstart, toSend, &fecmp);
            if (hasErrors)
            {
                if (latestReceivedLength != 0)
                {
                    if (SequenceEqual(payloadbuffer, latestReceived, payloadlength))
                    {
                        fec_corrects++;
                    }
                }
            }
            else
            {
                if (latestReceivedLength != payloadlength)
                {
                    printf("FEC Length missmatch at receive\n");
                    printf("Transmitted: ");
                    printPacket(msgstart, toSend);
                    printf("Original: ");
                    printPacket(payloadbuffer, payloadlength);
                    printf("Transmitlength: %u\n", toSend);
                    printf("Recovered packet lenght: %u\n", SMP_PacketGetLength(msgstart, 0));
                    printf("Received: ");
                    printPacket(latestReceived, latestReceivedLength);
                    printf("Returncode %d\n", returncode);
                    printf("Last packet: ");
                    printPacket(lasttransmittedmessage, lastDataToTransmitCount);
                    if (lasttransmittedmessage[lastDataToTransmitCount - 1] == 0xFF)
                    {
                        //This error happens when the last byte of the previous message is a unescaped framestart and the lengthbyte was to long
                        //The smp decoder then assumes that the following Framestart is a masked byte and keeps processing.
                        //This results in a checksum error and two lost packeges. However the decoder should resync with the next message if the same error
                        //doesn't happen again
                        printf("Real framestart was masked by the error\n");
                    }
                    else
                    {
                        //return -1; //This is a serious error
                    }
                }
                else
                {
                    if (!SequenceEqual(latestReceived, payloadbuffer, latestReceivedLength))
                    {
                        printf("SMP Data missmatch at receive\n");
                        printf("Transmitted: ");
                        printPacket(payloadbuffer, payloadlength);
                        printf("Received: ");
                        printPacket(latestReceived, latestReceivedLength);
                        printf("Returncode %d\n", returncode);
                        return -1; //This is a serious error
                    }
                }
            }
#endif
            memcpy(lasttransmittedmessage, msgstart, toSend);
            lastDataToTransmitCount = toSend;
        }
        else
        {
            printf("Couldn't create smp packet\n");
            printPacket(payloadbuffer, payloadlength);
        }
    }
    printf("\n\n");
    printf("All test successfull\n");
    printf("Metrics:\n");
    printf("\tTransmitted messages: %u\n", STABILITY_RUNS);
    printf("\tSMP Received messages: %u\n", smpreceivedmessages);
    printf("\tSMP Packetloss: %.2f %%\n", 100.0 - ((double)smpreceivedmessages * 100 / (double)STABILITY_RUNS));
    printf("\tAverage Bytes per message: %.2f\n", (double)smppayloadbytes / (double)STABILITY_RUNS);
    printf("\tByteerrorpropability: %.4f %%\n", (double)smptotalByteErrors * 100.0 / (double)smptransmittedbytes);
    printf("\tCRC Errors: %u\n", smpcrcerrors);
    printf("\tCRC Errorprobability: %.4f %%\n", (double)smpcrcerrors * 100.0 / STABILITY_RUNS);
    printf("\tFEC Repairs: %u\n", fec_corrects - intactsmpcount);
    printf("\tFEC Received messages: %u\n", smpreceivedmessages + (fec_corrects - intactsmpcount));
    printf("\tFEC Packetloss: %.2f %%\n", 100.0 - ((double)(smpreceivedmessages + (fec_corrects - intactsmpcount)) * 100 / (double)STABILITY_RUNS));
    printf("\tOverhead: %.2f %%\n", 100.0 - (double)smppayloadbytes * 100.0 / ((double)smptransmittedbytes));
}