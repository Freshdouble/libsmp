#include "../fecmp/ecc.h"

#include <stdbool.h>

#define HEADER_LENGTH 3
#define HEADER_BLOCK_LENGTH (HEADER_LENGTH + NPAR)

static inline bool checkHeaderChecksum(uint8_t *header, uint8_t receivedchecksum)
{
    return receivedchecksum == (header[0] ^ header[1]);
}

int main()
{
    ecc_t fec;
    uint16_t blockcount = 0;
    initialize_ecc(&fec);
    uint8_t headerfieldstart[HEADER_BLOCK_LENGTH];
    uint16_t blockcounter = 4;
    headerfieldstart[0] = (blockcounter >> 8) & 0xFF;
    headerfieldstart[1] = blockcounter & 0xFF;
    headerfieldstart[2] = headerfieldstart[0] ^ headerfieldstart[1]; //Additional header checksum

    encode_calcParity(headerfieldstart, HEADER_LENGTH, headerfieldstart + HEADER_LENGTH, &fec);

    bool valid;
    //Check syndroms
    decode_data(headerfieldstart, HEADER_BLOCK_LENGTH, &fec);
    valid = false;
    if (check_syndrome(&fec))
    {
        //We have errors
        if (correct_errors_erasures(headerfieldstart, HEADER_BLOCK_LENGTH, 0, 0, &fec))
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
        if (checkHeaderChecksum(headerfieldstart, headerfieldstart[2]))
        {
            blockcount = headerfieldstart[0] << 8 | headerfieldstart[1];
        }
    }
    return 0;
}