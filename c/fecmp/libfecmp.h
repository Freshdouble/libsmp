#pragma once

#include "../libsmp.h"
#include "ecc.h"
#include <stdbool.h>
#include "../sharedlib.h"

#define BLOCKSIZE 20

#if BLOCKSIZE - NPAR < 6
#error "The datasize per block must be at least 6"
#endif

/***
 * The length of the buffer should be a multiple of the blocksize to ensure the whole buffer could be used
 * */
typedef struct
{
    uint8_t* data;
    uint32_t bufferlength;
    SMP_Frame_Ready frameReadyCallback;
}libfecmp_settings_t;

typedef struct 
{
    uint8_t* writeptr;
    struct
    {
        unsigned int decoderstate : 2;
    }flags;
    uint8_t lastCorrelationByte;
    uint32_t bytesToReceive;
    uint16_t blockcount;
    uint8_t datainlastblock;
}libfecmp_status_t;


typedef struct 
{
    libfecmp_settings_t settings;
    libfecmp_status_t status;
    ecc_t ecc;
    smp_struct_t smp;
}libfecmp_t;

#ifdef CREATE_ALLOC_LAYER
MODULE_API libfecmp_t* fec_CreateInstance(uint32_t bufferlength, SMP_Frame_Ready frameready);
MODULE_API void fec_DestroyInstance(libfecmp_t *lf);
#endif

MODULE_API int fec_Init(libfecmp_t* lf, const libfecmp_settings_t* settings);
MODULE_API int fec_encode(const uint8_t* msg, uint32_t msglength, uint8_t* packet, uint32_t maxpacketlength, uint8_t** startptr, libfecmp_t* lf);
MODULE_API int fec_processByte(uint8_t data, libfecmp_t *lf);
MODULE_API void fec_processBytes(uint8_t* data, uint32_t length, libfecmp_t *lf);
