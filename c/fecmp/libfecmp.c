#include "libfecmp.h"
#include <string.h>
#ifndef NOASSERT
#include <assert.h>
#endif

#define CALC_BLOCKS(datalength, blocklength) (datalength / blocklength)
#define DATA_PER_BLOCK (BLOCKSIZE - NPAR)
#define CORRELATIONTHRESHOLD 9

#define HEADER_LENGTH 3
#define HEADER_BLOCK_LENGTH (HEADER_LENGTH + NPAR)

const uint8_t barkercode[] = {0x1F, 0x35};
const uint8_t barkerbitlength = 13;

#define HEADER_OFFSET sizeof(barkercode)
#define PAYLOAD_OFFSET HEADER_OFFSET + HEADER_BLOCK_LENGTH

static inline bool checkHeaderChecksum(uint8_t *header, uint8_t receivedchecksum)
{
    return receivedchecksum == (header[0] ^ header[1]);
}

static bool calculateCorrelation(uint8_t data, libfecmp_t *lf)
{
    const uint8_t highbytemask = 0x1F;
    uint8_t highbyte = lf->status.lastCorrelationByte;
    lf->status.lastCorrelationByte = data;
    //Exor the received code with the inverted barker code
    //If both are the same val would be all ones
    uint8_t correlationCounter = 0;
    uint8_t invertedBarkerHighByte = ~barkercode[0];
    uint8_t invertedBarkerLowByte = ~barkercode[1];
    uint16_t val = (highbyte ^ invertedBarkerHighByte) << 8 | (data ^ invertedBarkerLowByte);
    //count the ones
    for (int i = 0; i < (sizeof(barkercode) * 8); i++)
    {
        uint32_t mask = 1 << i;
        if (val & mask)
        {
            correlationCounter++;
        }
    }

    if (correlationCounter > CORRELATIONTHRESHOLD)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void resetDecoderstate(libfecmp_t *lf)
{
    memset(&lf->status, 0, sizeof(libfecmp_status_t));
    lf->status.writeptr = lf->settings.data;
    lf->status.lastCorrelationByte = ~barkercode[0]; //Initialize with inverted barker for minmal correlation on the first byte
}

int fec_Init(libfecmp_t *lf, const libfecmp_settings_t *settings)
{
    resetDecoderstate(lf);
    memcpy(&lf->settings, settings, sizeof(libfecmp_settings_t));
    if (lf->settings.data == 0) //This is not possible so we return a error
    {
        return -1;
    }
    initialize_ecc(&lf->ecc);

    //Initialize the smp stack
    smp_settings_t smpsettings;
    memset(&smpsettings, 0, sizeof(smp_settings_t));
    smpsettings.buffer.buffer = &lf->settings.data[PAYLOAD_OFFSET];
    smpsettings.buffer.maxlength = lf->settings.bufferlength - (PAYLOAD_OFFSET);
    smpsettings.frameReadyCallback = lf->settings.frameReadyCallback;
    if (SMP_Init(&lf->smp, &smpsettings) < 0)
        return -1;
    return 0;
}

int fec_encode(const uint8_t *msg, uint32_t msglength, uint8_t *packet, uint32_t maxpacketlength, uint8_t **startptr, libfecmp_t *lf)
{
    if ((CALC_BLOCKS(msglength, BLOCKSIZE) + 1) * BLOCKSIZE + sizeof(barkercode) > maxpacketlength)
    {
        return -1; //The array is not long enough to store the packet
    }

    uint8_t *msgstart;
    uint8_t *packetarrayend = &packet[maxpacketlength];
    //create smppacket first
    uint32_t smplength = SMP_Send(msg, msglength, &packet[PAYLOAD_OFFSET], maxpacketlength - BLOCKSIZE, &msgstart);
    if (smplength > 0xFE * DATA_PER_BLOCK)
    {
        //This is the maximum bytenumber we can handle
        return -2;
    }
    if (smplength == 0)
    {
        //We couldn't create the smppacket
        return -1;
    }
    //Calculate the new startindex of the message
    uint8_t *fecmessagestart = msgstart - (sizeof(barkercode) + HEADER_BLOCK_LENGTH);
    uint8_t *headerfieldstart = fecmessagestart + sizeof(barkercode);
    //Copy the barkercode
    memcpy(fecmessagestart, barkercode, sizeof(barkercode));
    //First Block is calculated at last so we skip it first
    //The first block also contains the total blocksize of the message
    uint8_t *payloadstart = headerfieldstart + HEADER_BLOCK_LENGTH;
#ifndef NOASSERT
    assert(payloadstart == msgstart);
#endif
    uint8_t blockcounter = 1 + ((smplength - 1) / DATA_PER_BLOCK); //Integer implementation of ceil function
    uint8_t *fecparityptr = payloadstart + smplength;
    uint32_t remainingbytes = smplength;
    //We start with the data after the first fec block
    for (uint8_t i = 0; i < blockcounter - 1; i++)
    {
        if ((fecparityptr + NPAR) >= packetarrayend)
        {
            //We dont have enough space to store the packet
            return -2;
        }
        //Calculate the fec data for the remaining fec blocks
        encode_calcParity(&msgstart[i * DATA_PER_BLOCK], DATA_PER_BLOCK, fecparityptr, &lf->ecc);
        fecparityptr += NPAR;
        remainingbytes -= DATA_PER_BLOCK;
    }
//Handle the last block
#ifndef NOASSERT
    assert(remainingbytes <= DATA_PER_BLOCK);
#endif
    if (remainingbytes > 0)
    {
        encode_calcParity(&msgstart[(blockcounter - 1) * DATA_PER_BLOCK], remainingbytes, fecparityptr, &lf->ecc);
    }
    //Write blockcount
    headerfieldstart[0] = remainingbytes & 0xFF;
    headerfieldstart[1] = blockcounter;
    headerfieldstart[2] = headerfieldstart[0] ^ headerfieldstart[1]; //Additional header checksum
    //Calculate the fec for the first block
    encode_calcParity(headerfieldstart, HEADER_LENGTH, headerfieldstart + HEADER_LENGTH, &lf->ecc);
    *startptr = fecmessagestart;
    return (fecparityptr + NPAR) - fecmessagestart;
}

static inline uint16_t checkHeaderBlock(libfecmp_t *lf)
{
    bool valid;
    //Check syndroms
    decode_data(lf->settings.data + HEADER_OFFSET, HEADER_BLOCK_LENGTH, &lf->ecc);
    valid = false;
    if (check_syndrome(&lf->ecc))
    {
        //We have errors
        if (correct_errors_erasures(lf->settings.data + HEADER_OFFSET, HEADER_BLOCK_LENGTH, 0, 0, &lf->ecc))
        {
            valid = true;
        }
    }
    else
    {
        //No errors detected
        valid = true;
    }
    if (valid)
    {
        //We have corrected the errors
        if (checkHeaderChecksum(lf->settings.data + HEADER_OFFSET, lf->settings.data[HEADER_OFFSET + 2]))
        {
            lf->status.datainlastblock = lf->settings.data[HEADER_OFFSET];
            lf->status.blockcount = lf->settings.data[HEADER_OFFSET + 1];
            return lf->status.blockcount;
        }
    }
    return 0;
}

static inline void gotoPayloadreception(libfecmp_t *lf, uint16_t blockcount)
{
    uint8_t datainlastblock = lf->status.datainlastblock;
    memset(&lf->status, 0, sizeof(libfecmp_status_t));
    lf->status.datainlastblock = datainlastblock;
    lf->status.writeptr = lf->settings.data + PAYLOAD_OFFSET;
    lf->status.flags.decoderstate = 2;
    lf->status.bytesToReceive = BLOCKSIZE * blockcount - (DATA_PER_BLOCK - lf->status.datainlastblock);
    lf->status.blockcount = blockcount;
}

static void fec_processByte(uint8_t data, libfecmp_t *lf)
{
    bool valid;
    bool wasreceiving;
    uint16_t blocklength;
    switch (lf->status.flags.decoderstate)
    {
    case 0: //we wait for a framestart
        if (calculateCorrelation(data, lf))
        {
            lf->status.flags.decoderstate = 1;
            lf->status.bytesToReceive = HEADER_BLOCK_LENGTH;
            lf->status.writeptr = lf->settings.data + HEADER_OFFSET;
        }
        //Store the last HEADER_BLOCK_LENGTH bytes
        for (int i = 1; i < HEADER_BLOCK_LENGTH; i++)
        {
            lf->settings.data[HEADER_OFFSET + i - 1] = lf->settings.data[HEADER_OFFSET + i];
        }
        lf->settings.data[HEADER_OFFSET + HEADER_BLOCK_LENGTH - 1] = data;
        //Forward all bytes comming from the interface to the smp parser to detect raw smp messages
        wasreceiving = lf->smp.status.flags.recieving;
        SMP_RecieveInByte(data, &lf->smp);
        if (lf->smp.status.flags.recieving && !wasreceiving && lf->status.flags.decoderstate == 0)
        {
            //If the last byte switched the smp receiver from idle to receiving
            //Calculate the checksum over the last HEADER_BLOCK_LENGTH bytes.
            //If the block is a valid rs block we missed the framestart so we start receiving now
            blocklength = checkHeaderBlock(lf);
            if (blocklength > 0)
            {
                //The block was valid
                gotoPayloadreception(lf, blocklength);
                //Reset the smp decoder, libfecmp handles this packet
                SMP_ResetDecoderState(&lf->smp, false);
            }
        }
        break;
    case 1: //We receive the first fec block
        lf->status.bytesToReceive--;
        if (lf->status.writeptr == &lf->settings.data[lf->settings.bufferlength])
        {
            //Bufferoverflow reset decoder and stop decoding
            resetDecoderstate(lf);
        }
        *lf->status.writeptr = data;
        lf->status.writeptr++;
        if (lf->status.bytesToReceive == 0)
        {
            blocklength = checkHeaderBlock(lf);
            if (blocklength > 0)
            {
                SMP_ResetDecoderState(&lf->smp, false);
                gotoPayloadreception(lf, blocklength);
            }
            else
            {
                lf->status.flags.decoderstate = 0; // The block was not valid. However it could still be a smp packet
                SMP_RecieveInBytes(lf->settings.data, HEADER_BLOCK_LENGTH, &lf->smp);
            }
        }
        break;
    case 2:
        //Receive all bytes
        lf->status.bytesToReceive--;
        if (lf->status.writeptr == &lf->settings.data[lf->settings.bufferlength])
        {
            //Bufferoverflow reset decoder and stop decoding
            resetDecoderstate(lf);
        }
        *lf->status.writeptr = data;
        lf->status.writeptr++;
        if (lf->status.bytesToReceive == 0)
        {
            //We have received all bytes
            if (lf->status.datainlastblock > DATA_PER_BLOCK)
            {
                lf->status.datainlastblock = DATA_PER_BLOCK;
            }
            //Start decoding
            uint8_t *dataptr = lf->settings.data + PAYLOAD_OFFSET;
            uint8_t *fecptr = lf->settings.data + PAYLOAD_OFFSET + (DATA_PER_BLOCK * (lf->status.blockcount - 1) + lf->status.datainlastblock);
            uint8_t currentblock[BLOCKSIZE];
            valid = true;
            SMP_ResetDecoderState(&lf->smp, false);
            //Iterate over all blocks
            for (uint16_t i = 0; i < lf->status.blockcount - 1; i++)
            {
                memcpy(currentblock, dataptr, DATA_PER_BLOCK);
                memcpy(&currentblock[DATA_PER_BLOCK], fecptr, NPAR);
                decode_data(currentblock, BLOCKSIZE, &lf->ecc);
                if (check_syndrome(&lf->ecc))
                {
                    if (correct_errors_erasures(currentblock, BLOCKSIZE, 0, 0, &lf->ecc))
                    {
                        memcpy(dataptr, currentblock, DATA_PER_BLOCK);
                    }
                }
                SMP_RecieveInBytes(currentblock, DATA_PER_BLOCK, &lf->smp);
                dataptr += DATA_PER_BLOCK;
                fecptr += NPAR;
            }
            //Handle last block
            if (lf->status.datainlastblock > 0)
            {
                memcpy(currentblock, dataptr, lf->status.datainlastblock);
                memcpy(&currentblock[lf->status.datainlastblock], fecptr, NPAR);
                decode_data(currentblock, lf->status.datainlastblock + NPAR, &lf->ecc);
                if (check_syndrome(&lf->ecc))
                {
                    if (correct_errors_erasures(currentblock, lf->status.datainlastblock + NPAR, 0, 0, &lf->ecc))
                    {
                        memcpy(dataptr, currentblock, lf->status.datainlastblock);
                    }
                }
                SMP_RecieveInBytes(currentblock, lf->status.datainlastblock, &lf->smp);
            }
            lf->status.flags.decoderstate = 0;
        }
        break;
    }
}

void fec_processBytes(uint8_t *data, uint32_t length, libfecmp_t *lf)
{
    for (uint32_t i = 0; i < length; i++)
    {
        fec_processByte(data[i], lf);
    }
}