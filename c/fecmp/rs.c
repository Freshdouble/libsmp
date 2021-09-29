/*
 * Reed Solomon Encoder/Decoder
 *
 * Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009
 *
 * This software library is licensed under terms of the GNU GENERAL
 * PUBLIC LICENSE
 *
 * RSCODE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RSCODE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rscode.  If not, see <http://www.gnu.org/licenses/>.

 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 * Source code is available at http://rscode.sourceforge.net
 */

#include <stdio.h>
#include <ctype.h>
#include "ecc.h"

static void
compute_genpoly(int nbytes, int genpoly[], ecc_t *ecc);

/* Initialize lookup tables, polynomials, etc. */
void initialize_ecc(ecc_t *ecc)
{
  /* Initialize the galois field arithmetic tables */
  init_galois_tables(ecc);

  /* Compute the encoder generator polynomial */
  compute_genpoly(NPAR, ecc->genPoly, ecc);
}

void zero_fill_from(unsigned char buf[], int from, int to)
{
  int i;
  for (i = from; i < to; i++)
    buf[i] = 0;
}

/* debugging routines */
void print_parity(ecc_t *ecc)
{
  int i;
  printf("Parity Bytes: ");
  for (i = 0; i < NPAR; i++)
    printf("[%d]:%x, ", i, ecc->pBytes[i]);
  printf("\n");
}

void print_syndrome(ecc_t *ecc)
{
  int i;
  printf("Syndrome Bytes: ");
  for (i = 0; i < NPAR; i++)
    printf("[%d]:%x, ", i, ecc->synBytes[i]);
  printf("\n");
}

/* Append the parity bytes onto the end of the message */
void build_codeword(unsigned char msg[], int nbytes, unsigned char dst[], ecc_t *ecc)
{
  int i;

  for (i = 0; i < nbytes; i++)
    dst[i] = msg[i];

  for (i = 0; i < NPAR; i++)
  {
    dst[i + nbytes] = ecc->pBytes[NPAR - 1 - i];
  }
}

/**********************************************************
 * Reed Solomon Decoder
 *
 * Computes the syndrome of a codeword. Puts the results
 * into the synBytes[] array.
 */

void decode_data(unsigned char data[], int nbytes, ecc_t *ecc)
{
  int i, j, sum;
  for (j = 0; j < NPAR; j++)
  {
    sum = 0;
    for (i = 0; i < nbytes; i++)
    {
      sum = data[i] ^ gmult(ecc->gexp[j + 1], sum, ecc);
    }
    ecc->synBytes[j] = sum;
  }
}

/* Check if the syndrome is zero */
int check_syndrome(ecc_t *ecc)
{
  int i, nz = 0;
  for (i = 0; i < NPAR; i++)
  {
    if (ecc->synBytes[i] != 0)
    {
      nz = 1;
      break;
    }
  }
  return nz;
}

void debug_check_syndrome(ecc_t *ecc)
{
  int i;

  for (i = 0; i < 3; i++)
  {
    printf(" inv log S[%d]/S[%d] = %d\n", i, i + 1,
           ecc->glog[gmult(ecc->synBytes[i], ginv(ecc->synBytes[i + 1], ecc), ecc)]);
  }
}

/* Create a generator polynomial for an n byte RS code.
 * The coefficients are returned in the genPoly arg.
 * Make sure that the genPoly array which is passed in is
 * at least n+1 bytes long.
 */

static void
compute_genpoly(int nbytes, int genpoly[], ecc_t *ecc)
{
  int i, tp[256], tp1[256];

  /* multiply (x + a^n) for n = 1 to nbytes */

  zero_poly(tp1);
  tp1[0] = 1;

  for (i = 1; i <= nbytes; i++)
  {
    zero_poly(tp);
    tp[0] = ecc->gexp[i]; /* set up x+a^n */
    tp[1] = 1;

    mult_polys(genpoly, tp, tp1, ecc);
    copy_poly(tp1, genpoly);
  }
}

/* Simulate a LFSR with generator polynomial for n byte RS code.
 * Pass in a pointer to the data array, and amount of data.
 *
 * The parity bytes are deposited into pBytes[], and the whole message
 * and parity are copied to dest to make a codeword.
 *
 */

void encode_data(unsigned char msg[], int nbytes, unsigned char dst[], ecc_t *ecc)
{
  int i, LFSR[NPAR + 1], dbyte, j;

  for (i = 0; i < NPAR + 1; i++)
    LFSR[i] = 0;

  for (i = 0; i < nbytes; i++)
  {
    dbyte = msg[i] ^ LFSR[NPAR - 1];
    for (j = NPAR - 1; j > 0; j--)
    {
      LFSR[j] = LFSR[j - 1] ^ gmult(ecc->genPoly[j], dbyte, ecc);
    }
    LFSR[0] = gmult(ecc->genPoly[0], dbyte, ecc);
  }

  for (i = 0; i < NPAR; i++)
    ecc->pBytes[i] = LFSR[i];

  build_codeword(msg, nbytes, dst, ecc);
}

/************************
 * @brief Calculates the parity bytes for a rs block
 * @param msg The messageblock to encode
 * @param nbytes Number of payloadbytes in the block, also the length of msg
 * @param parity Array that holds the calculated paritybytes. Must be at least NPAR long or zero if omitted
 * @param ecc Pointer to the ecc status structure
 * */
void encode_calcParity(unsigned char msg[], int nbytes, unsigned char parity[],ecc_t *ecc)
{
  //TODO: Merge with encode_data
  int i, LFSR[NPAR + 1], dbyte, j;

  for (i = 0; i < NPAR + 1; i++)
    LFSR[i] = 0;
  
  for (i = 0; i < nbytes; i++)
  {
    dbyte = msg[i] ^ LFSR[NPAR - 1];
    for (j = NPAR - 1; j > 0; j--)
    {
      LFSR[j] = LFSR[j - 1] ^ gmult(ecc->genPoly[j], dbyte, ecc);
    }
    LFSR[0] = gmult(ecc->genPoly[0], dbyte, ecc);
  }
  for (i = 0; i < NPAR; i++)
    ecc->pBytes[i] = LFSR[i];
  if(parity != 0)
  {
    for (i = 0; i < NPAR; i++)
    {
      parity[i] = ecc->pBytes[NPAR - 1 - i];
    }
  }
}

int copyParity(uint8_t* buffer, uint32_t length, ecc_t *ecc)
{
  if(length < NPAR) return -1;
  for (int i = 0; i < NPAR; i++)
  {
    buffer[i] = ecc->pBytes[NPAR - 1 - i];
  }
  return 0;
}