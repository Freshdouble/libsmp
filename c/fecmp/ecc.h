/* Reed Solomon Coding for glyphs
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
 *
 * Source code is available at http://rscode.sourceforge.net
 *
 * Commercial licensing is available under a separate license, please
 * contact author for details.
 *
 */

#ifndef ECC_H__
#define ECC_H__

#ifdef __cplusplus
extern "C"
{
#endif

  /****************************************************************

  Below is NPAR, the only compile-time parameter you should have to
  modify.

  It is the number of parity bytes which will be appended to
  your data to create a codeword.

  Note that the maximum codeword size is 255, so the
  sum of your message length plus parity should be less than
  or equal to this maximum limit.

  In practice, you will get slooow error correction and decoding
  if you use more than a reasonably small number of parity bytes.
  (say, 10 or 20)

  ****************************************************************/
#ifndef NPAR
#define NPAR 4
#endif

  /****************************************************************/

#define TRUE 1
#define FALSE 0

#include <stdint.h>

  typedef uint32_t BIT32;
  typedef uint16_t BIT16;

/* **************************************************************** */

/* Maximum degree of various polynomials. */
#define MAXDEG (NPAR * 2)

  /*************************************/

  typedef struct
  {
    int gexp[512];
    int glog[256];
    /* Encoder parity bytes */
    int pBytes[MAXDEG];

    /* Decoder syndrome bytes */
    int synBytes[MAXDEG];

    /* generator polynomial */
    int genPoly[MAXDEG * 2];

    /* The Error Locator Polynomial, also known as Lambda or Sigma. Lambda[0] == 1 */
    int Lambda[MAXDEG];
    /* The Error Evaluator Polynomial */
    int Omega[MAXDEG];

    /* error locations found using Chien's search*/
    int ErrorLocs[256];
    int NErrors;

    /* erasure flags */
    int ErasureLocs[256];
    int NErasures;
  } ecc_t;

  /* Reed Solomon encode/decode routines */
  void initialize_ecc(ecc_t *ecc);
  int check_syndrome(ecc_t *ecc);
  void decode_data(unsigned char data[], int nbytes, ecc_t *ecc);
  void encode_calcParity(unsigned char msg[], int nbytes, unsigned char parity[],ecc_t *ecc);
  int copyParity(uint8_t* buffer, uint32_t length, ecc_t *ecc);
  void encode_data(unsigned char msg[], int nbytes, unsigned char dst[], ecc_t *ecc);

  /* CRC-CCITT checksum generator */
  BIT16 crc_ccitt(unsigned char *msg, int len);

  void init_galois_tables(ecc_t *ecc);
  int ginv(int elt, ecc_t *ecc);
  int gmult(int a, int b, ecc_t *ecc);

  /* Error location routines */
  int correct_errors_erasures(unsigned char codeword[], int csize, int nerasures, int erasures[], ecc_t *ecc);

  /* polynomial arithmetic */
  void add_polys(int dst[], int src[]);
  void scale_poly(int k, int poly[], ecc_t *ecc);
  void mult_polys(int dst[], int p1[], int p2[], ecc_t *ecc);

  void copy_poly(int dst[], int src[]);
  void zero_poly(int poly[]);

#ifdef __cplusplus
}
#endif
#endif // ECC_H__
