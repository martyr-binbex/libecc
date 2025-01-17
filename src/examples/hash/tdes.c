/*
 *  Copyright (C) 2021 - This file is part of libecc project
 *
 *  Authors:
 *      Ryad BENADJILA <ryadbenadjila@gmail.com>
 *      Arnaud EBALARD <arnaud.ebalard@ssi.gouv.fr>
 *
 *  This software is licensed under a dual BSD and GPL v2 license.
 *  See LICENSE file at the root folder of the project.
 */

#include "tdes.h"

/* This is a very straightforward and basic implementation of DES and T-DES */

/* platform-independant 32-bit integer manipulation macros */
#ifndef GET_UINT32
#define GET_UINT32(n,b,i)                             \
do {                                                  \
    (n) = ( (unsigned int) (b)[(i)    ] << 24 )       \
        | ( (unsigned int) (b)[(i) + 1] << 16 )       \
        | ( (unsigned int) (b)[(i) + 2] <<  8 )       \
        | ( (unsigned int) (b)[(i) + 3]       );      \
} while( 0 )
#endif

#ifndef PUT_UINT32
#define PUT_UINT32(n,b,i)                               \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )
#endif

/* DES 8 S-Boxes */
static const unsigned int SB[8][64] = {
  {
    0x01010400, 0x00000000, 0x00010000, 0x01010404,
    0x01010004, 0x00010404, 0x00000004, 0x00010000,
    0x00000400, 0x01010400, 0x01010404, 0x00000400,
    0x01000404, 0x01010004, 0x01000000, 0x00000004,
    0x00000404, 0x01000400, 0x01000400, 0x00010400,
    0x00010400, 0x01010000, 0x01010000, 0x01000404,
    0x00010004, 0x01000004, 0x01000004, 0x00010004,
    0x00000000, 0x00000404, 0x00010404, 0x01000000,
    0x00010000, 0x01010404, 0x00000004, 0x01010000,
    0x01010400, 0x01000000, 0x01000000, 0x00000400,
    0x01010004, 0x00010000, 0x00010400, 0x01000004,
    0x00000400, 0x00000004, 0x01000404, 0x00010404,
    0x01010404, 0x00010004, 0x01010000, 0x01000404,
    0x01000004, 0x00000404, 0x00010404, 0x01010400,
    0x00000404, 0x01000400, 0x01000400, 0x00000000,
    0x00010004, 0x00010400, 0x00000000, 0x01010004
  },
  {
    0x80108020, 0x80008000, 0x00008000, 0x00108020,
    0x00100000, 0x00000020, 0x80100020, 0x80008020,
    0x80000020, 0x80108020, 0x80108000, 0x80000000,
    0x80008000, 0x00100000, 0x00000020, 0x80100020,
    0x00108000, 0x00100020, 0x80008020, 0x00000000,
    0x80000000, 0x00008000, 0x00108020, 0x80100000,
    0x00100020, 0x80000020, 0x00000000, 0x00108000,
    0x00008020, 0x80108000, 0x80100000, 0x00008020,
    0x00000000, 0x00108020, 0x80100020, 0x00100000,
    0x80008020, 0x80100000, 0x80108000, 0x00008000,
    0x80100000, 0x80008000, 0x00000020, 0x80108020,
    0x00108020, 0x00000020, 0x00008000, 0x80000000,
    0x00008020, 0x80108000, 0x00100000, 0x80000020,
    0x00100020, 0x80008020, 0x80000020, 0x00100020,
    0x00108000, 0x00000000, 0x80008000, 0x00008020,
    0x80000000, 0x80100020, 0x80108020, 0x00108000
  },
  {
    0x00000208, 0x08020200, 0x00000000, 0x08020008,
    0x08000200, 0x00000000, 0x00020208, 0x08000200,
    0x00020008, 0x08000008, 0x08000008, 0x00020000,
    0x08020208, 0x00020008, 0x08020000, 0x00000208,
    0x08000000, 0x00000008, 0x08020200, 0x00000200,
    0x00020200, 0x08020000, 0x08020008, 0x00020208,
    0x08000208, 0x00020200, 0x00020000, 0x08000208,
    0x00000008, 0x08020208, 0x00000200, 0x08000000,
    0x08020200, 0x08000000, 0x00020008, 0x00000208,
    0x00020000, 0x08020200, 0x08000200, 0x00000000,
    0x00000200, 0x00020008, 0x08020208, 0x08000200,
    0x08000008, 0x00000200, 0x00000000, 0x08020008,
    0x08000208, 0x00020000, 0x08000000, 0x08020208,
    0x00000008, 0x00020208, 0x00020200, 0x08000008,
    0x08020000, 0x08000208, 0x00000208, 0x08020000,
    0x00020208, 0x00000008, 0x08020008, 0x00020200
  },
  {
    0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802080, 0x00800081, 0x00800001, 0x00002001,
    0x00000000, 0x00802000, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00800080, 0x00800001,
    0x00000001, 0x00002000, 0x00800000, 0x00802001,
    0x00000080, 0x00800000, 0x00002001, 0x00002080,
    0x00800081, 0x00000001, 0x00002080, 0x00800080,
    0x00002000, 0x00802080, 0x00802081, 0x00000081,
    0x00800080, 0x00800001, 0x00802000, 0x00802081,
    0x00000081, 0x00000000, 0x00000000, 0x00802000,
    0x00002080, 0x00800080, 0x00800081, 0x00000001,
    0x00802001, 0x00002081, 0x00002081, 0x00000080,
    0x00802081, 0x00000081, 0x00000001, 0x00002000,
    0x00800001, 0x00002001, 0x00802080, 0x00800081,
    0x00002001, 0x00002080, 0x00800000, 0x00802001,
    0x00000080, 0x00800000, 0x00002000, 0x00802080
  },
  {
    0x00000100, 0x02080100, 0x02080000, 0x42000100,
    0x00080000, 0x00000100, 0x40000000, 0x02080000,
    0x40080100, 0x00080000, 0x02000100, 0x40080100,
    0x42000100, 0x42080000, 0x00080100, 0x40000000,
    0x02000000, 0x40080000, 0x40080000, 0x00000000,
    0x40000100, 0x42080100, 0x42080100, 0x02000100,
    0x42080000, 0x40000100, 0x00000000, 0x42000000,
    0x02080100, 0x02000000, 0x42000000, 0x00080100,
    0x00080000, 0x42000100, 0x00000100, 0x02000000,
    0x40000000, 0x02080000, 0x42000100, 0x40080100,
    0x02000100, 0x40000000, 0x42080000, 0x02080100,
    0x40080100, 0x00000100, 0x02000000, 0x42080000,
    0x42080100, 0x00080100, 0x42000000, 0x42080100,
    0x02080000, 0x00000000, 0x40080000, 0x42000000,
    0x00080100, 0x02000100, 0x40000100, 0x00080000,
    0x00000000, 0x40080000, 0x02080100, 0x40000100
  },
  {
    0x20000010, 0x20400000, 0x00004000, 0x20404010,
    0x20400000, 0x00000010, 0x20404010, 0x00400000,
    0x20004000, 0x00404010, 0x00400000, 0x20000010,
    0x00400010, 0x20004000, 0x20000000, 0x00004010,
    0x00000000, 0x00400010, 0x20004010, 0x00004000,
    0x00404000, 0x20004010, 0x00000010, 0x20400010,
    0x20400010, 0x00000000, 0x00404010, 0x20404000,
    0x00004010, 0x00404000, 0x20404000, 0x20000000,
    0x20004000, 0x00000010, 0x20400010, 0x00404000,
    0x20404010, 0x00400000, 0x00004010, 0x20000010,
    0x00400000, 0x20004000, 0x20000000, 0x00004010,
    0x20000010, 0x20404010, 0x00404000, 0x20400000,
    0x00404010, 0x20404000, 0x00000000, 0x20400010,
    0x00000010, 0x00004000, 0x20400000, 0x00404010,
    0x00004000, 0x00400010, 0x20004010, 0x00000000,
    0x20404000, 0x20000000, 0x00400010, 0x20004010
  },
  {
    0x00200000, 0x04200002, 0x04000802, 0x00000000,
    0x00000800, 0x04000802, 0x00200802, 0x04200800,
    0x04200802, 0x00200000, 0x00000000, 0x04000002,
    0x00000002, 0x04000000, 0x04200002, 0x00000802,
    0x04000800, 0x00200802, 0x00200002, 0x04000800,
    0x04000002, 0x04200000, 0x04200800, 0x00200002,
    0x04200000, 0x00000800, 0x00000802, 0x04200802,
    0x00200800, 0x00000002, 0x04000000, 0x00200800,
    0x04000000, 0x00200800, 0x00200000, 0x04000802,
    0x04000802, 0x04200002, 0x04200002, 0x00000002,
    0x00200002, 0x04000000, 0x04000800, 0x00200000,
    0x04200800, 0x00000802, 0x00200802, 0x04200800,
    0x00000802, 0x04000002, 0x04200802, 0x04200000,
    0x00200800, 0x00000000, 0x00000002, 0x04200802,
    0x00000000, 0x00200802, 0x04200000, 0x00000800,
    0x04000002, 0x04000800, 0x00000800, 0x00200002
  },
  {
    0x10001040, 0x00001000, 0x00040000, 0x10041040,
    0x10000000, 0x10001040, 0x00000040, 0x10000000,
    0x00040040, 0x10040000, 0x10041040, 0x00041000,
    0x10041000, 0x00041040, 0x00001000, 0x00000040,
    0x10040000, 0x10000040, 0x10001000, 0x00001040,
    0x00041000, 0x00040040, 0x10040040, 0x10041000,
    0x00001040, 0x00000000, 0x00000000, 0x10040040,
    0x10000040, 0x10001000, 0x00041040, 0x00040000,
    0x00041040, 0x00040000, 0x10041000, 0x00001000,
    0x00000040, 0x10040040, 0x00001000, 0x00041040,
    0x10001000, 0x00000040, 0x10000040, 0x10040000,
    0x10040040, 0x10000000, 0x00040000, 0x10001040,
    0x00000000, 0x10041040, 0x00040040, 0x10000040,
    0x10040000, 0x10001000, 0x10001040, 0x00000000,
    0x10041040, 0x00041000, 0x00041000, 0x00001040,
    0x00001040, 0x00040040, 0x10000000, 0x10041000
  }
};

/* PC1: left and right halves bit-swap */

static const unsigned int LH[16] =
{
    0x00000000, 0x00000001, 0x00000100, 0x00000101,
    0x00010000, 0x00010001, 0x00010100, 0x00010101,
    0x01000000, 0x01000001, 0x01000100, 0x01000101,
    0x01010000, 0x01010001, 0x01010100, 0x01010101
};

static const unsigned int RH[16] =
{
    0x00000000, 0x01000000, 0x00010000, 0x01010000,
    0x00000100, 0x01000100, 0x00010100, 0x01010100,
    0x00000001, 0x01000001, 0x00010001, 0x01010001,
    0x00000101, 0x01000101, 0x00010101, 0x01010101,
};

/* DES Initial Permutation (IP) */
static inline void des_ip(unsigned int L[1], unsigned int R[1])
{
	unsigned int T;

	T = ((L[0] >>  4) ^ R[0]) & 0x0F0F0F0F; R[0] ^= T; L[0] ^= (T <<  4);
	T = ((L[0] >> 16) ^ R[0]) & 0x0000FFFF; R[0] ^= T; L[0] ^= (T << 16);
	T = ((R[0] >>  2) ^ L[0]) & 0x33333333; L[0] ^= T; R[0] ^= (T <<  2);
	T = ((R[0] >>  8) ^ L[0]) & 0x00FF00FF; L[0] ^= T; R[0] ^= (T <<  8);
	R[0] = ((R[0] << 1) | (R[0] >> 31)) & 0xFFFFFFFF;
	T = (L[0] ^ R[0]) & 0xAAAAAAAA; R[0] ^= T; L[0] ^= T;
	L[0] = ((L[0] << 1) | (L[0] >> 31)) & 0xFFFFFFFF;

	return;
}

/* DES Final Permutation (FP) */
static inline void des_fp(unsigned int L[1], unsigned int R[1])
{
	unsigned int T;

	L[0] = ((L[0] << 31) | (L[0] >> 1)) & 0xFFFFFFFF;
	T = (L[0] ^ R[0]) & 0xAAAAAAAA; L[0] ^= T; R[0] ^= T;
	R[0] = ((R[0] << 31) | (R[0] >> 1)) & 0xFFFFFFFF;
	T = ((R[0] >>  8) ^ L[0]) & 0x00FF00FF; L[0] ^= T; R[0] ^= (T <<  8);
	T = ((R[0] >>  2) ^ L[0]) & 0x33333333; L[0] ^= T; R[0] ^= (T <<  2);
	T = ((L[0] >> 16) ^ R[0]) & 0x0000FFFF; R[0] ^= T; L[0] ^= (T << 16);
	T = ((L[0] >>  4) ^ R[0]) & 0x0F0F0F0F; R[0] ^= T; L[0] ^= (T <<  4);

	return;
}

/* DES function: F(R, K) + L with inversion */
static inline void des_round(unsigned int L[1], unsigned int R[1], unsigned long long K)
{
	unsigned int T;
	unsigned int k1, k2;

	k1 = (unsigned int)K;
	k2 = (unsigned int)(K >> 32);

	T = k1 ^ L[0];
	R[0] ^= SB[7][ (T) & 0x3f] ^ SB[5][ (T >> 8) & 0x3f] ^ SB[3][ (T >> 16) & 0x3f] ^ SB[1][ (T >> 24) & 0x3f];
	T = k2 ^ ((L[0] << 28) | (L[0] >> 4));
	R[0] ^= SB[6][ (T) & 0x3f] ^ SB[4][ (T >> 8) & 0x3f] ^ SB[2][ (T >> 16) & 0x3f] ^ SB[0][ (T >> 24) & 0x3f];

	return;
}

/* DES key schedule */
ATTRIBUTE_WARN_UNUSED_RET int des_set_key(des_context *ctx, const unsigned char k[8], des_direction dir)
{
	unsigned int i;
	unsigned int C, D, T;
	int ret;

	if((ctx == NULL) || (k == NULL)){
		ret = -1;
		goto err;
	}

	ctx->dir = dir;

	GET_UINT32(C, k, 0);
	GET_UINT32(D, k, 4);

	/* Permuted choice 1 */
	T =  ((D >>  4) ^ C) & 0x0F0F0F0F;  C ^= T; D ^= (T <<  4);
	T =  ((D      ) ^ C) & 0x10101010;  C ^= T; D ^= (T      );

	C =   (LH[ (C      ) & 0xF] << 3) | (LH[ (C >>  8) & 0xF ] << 2)
	    | (LH[ (C >> 16) & 0xF] << 1) | (LH[ (C >> 24) & 0xF ]     )
	    | (LH[ (C >>  5) & 0xF] << 7) | (LH[ (C >> 13) & 0xF ] << 6)
	    | (LH[ (C >> 21) & 0xF] << 5) | (LH[ (C >> 29) & 0xF ] << 4);

	D =   (RH[ (D >>  1) & 0xF] << 3) | (RH[ (D >>  9) & 0xF ] << 2)
	    | (RH[ (D >> 17) & 0xF] << 1) | (RH[ (D >> 25) & 0xF ]     )
	    | (RH[ (D >>  4) & 0xF] << 7) | (RH[ (D >> 12) & 0xF ] << 6)
	    | (RH[ (D >> 20) & 0xF] << 5) | (RH[ (D >> 28) & 0xF ] << 4);

	C &= 0x0FFFFFFF;
	D &= 0x0FFFFFFF;

	/* Compute the subkeys */
	for( i = 0; i < 16; i++ ){
		unsigned int k1, k2;
		if((i < 2) || (i == 8) || (i == 15)){
			C = ((C <<  1) | (C >> 27)) & 0x0FFFFFFF;
			D = ((D <<  1) | (D >> 27)) & 0x0FFFFFFF;
		}
		else{
			C = ((C <<  2) | (C >> 26)) & 0x0FFFFFFF;
			D = ((D <<  2) | (D >> 26)) & 0x0FFFFFFF;
		}

		k1 =   ((C <<  4) & 0x24000000) | ((C << 28) & 0x10000000)
		   	| ((C << 14) & 0x08000000) | ((C << 18) & 0x02080000)
			| ((C <<  6) & 0x01000000) | ((C <<  9) & 0x00200000)
			| ((C >>  1) & 0x00100000) | ((C << 10) & 0x00040000)
			| ((C <<  2) & 0x00020000) | ((C >> 10) & 0x00010000)
			| ((D >> 13) & 0x00002000) | ((D >>  4) & 0x00001000)
			| ((D <<  6) & 0x00000800) | ((D >>  1) & 0x00000400)
			| ((D >> 14) & 0x00000200) | ((D      ) & 0x00000100)
			| ((D >>  5) & 0x00000020) | ((D >> 10) & 0x00000010)
			| ((D >>  3) & 0x00000008) | ((D >> 18) & 0x00000004)
			| ((D >> 26) & 0x00000002) | ((D >> 24) & 0x00000001);

		k2 =     ((C << 15) & 0x20000000) | ((C << 17) & 0x10000000)
		    	| ((C << 10) & 0x08000000) | ((C << 22) & 0x04000000)
		  	| ((C >>  2) & 0x02000000) | ((C <<  1) & 0x01000000)
		  	| ((C << 16) & 0x00200000) | ((C << 11) & 0x00100000)
		  	| ((C <<  3) & 0x00080000) | ((C >>  6) & 0x00040000)
		  	| ((C << 15) & 0x00020000) | ((C >>  4) & 0x00010000)
		  	| ((D >>  2) & 0x00002000) | ((D <<  8) & 0x00001000)
		  	| ((D >> 14) & 0x00000808) | ((D >>  9) & 0x00000400)
		  	| ((D      ) & 0x00000200) | ((D <<  7) & 0x00000100)
		  	| ((D >>  7) & 0x00000020) | ((D >>  3) & 0x00000011)
		  	| ((D <<  2) & 0x00000004) | ((D >> 21) & 0x00000002);

		if(dir == DES_ENCRYPTION){
			ctx->sk[i] = (((unsigned long long)k2) << 32) | (unsigned long long)k1;
		}
		else if(dir == DES_DECRYPTION){
			ctx->sk[15-i] = (((unsigned long long)k2) << 32) | (unsigned long long)k1;
		}
		else{
			ret = -1;
			goto err;
		}
	}

	ret = 0;

err:
	return ret;
}

/* DES encryption core */
ATTRIBUTE_WARN_UNUSED_RET static inline int des_core(const des_context *ctx, const unsigned char input[8], unsigned char output[8])
{
	unsigned int L, R;
	unsigned int i;
	int ret;

	if((ctx == NULL) || (input == NULL) || (output == NULL)){
		ret = -1;
		goto err;
	}

	GET_UINT32(L, input, 0);
	GET_UINT32(R, input, 4);

	des_ip(&L, &R);

	for(i = 0; i < 16; i++){
		if((i % 2) == 0){
			des_round(&R, &L, ctx->sk[i]);
		}
		else{
			des_round(&L, &R, ctx->sk[i]);
		}
	}

	des_fp(&R, &L);

	PUT_UINT32(R, output, 0);
	PUT_UINT32(L, output, 4);

	ret = 0;
err:
	return ret;
}

/* DES encryption/decryption */
ATTRIBUTE_WARN_UNUSED_RET int des(const des_context *ctx, const unsigned char input[8], unsigned char output[8])
{
	return des_core(ctx, input, output);
}

/* TDES key schedules */
ATTRIBUTE_WARN_UNUSED_RET int des3_set_keys(des3_context *ctx, const unsigned char k1[8], const unsigned char k2[8], const unsigned char k3[8], des_direction dir)
{
	int ret;

	if((ctx == NULL) || (k1 == NULL) || (k2 == NULL)){
		ret = -1;
		goto err;
	}
	ctx->dir = dir;
	if(dir == DES_ENCRYPTION){
		if(des_set_key(&(ctx->des[0]), k1, DES_ENCRYPTION)){
			ret = -1;
			goto err;
		}
		if(des_set_key(&(ctx->des[1]), k2, DES_DECRYPTION)){
			ret = -1;
			goto err;
		}
		if(k3 == NULL){
			if(des_set_key(&(ctx->des[2]), k1, DES_ENCRYPTION)){
				ret = -1;
				goto err;
			}
		}
		else{
			if(des_set_key(&(ctx->des[2]), k3, DES_ENCRYPTION)){
				ret = -1;
				goto err;
			}
		}
	}
	else if(dir == DES_DECRYPTION){
		if(des_set_key(&(ctx->des[0]), k1, DES_DECRYPTION)){
			ret = -1;
			goto err;
		}
		if(des_set_key(&(ctx->des[1]), k2, DES_ENCRYPTION)){
			ret = -1;
			goto err;
		}
		if(k3 == NULL){
			if(des_set_key(&(ctx->des[2]), k1, DES_DECRYPTION)){
				ret = -1;
				goto err;
			}
		}
		else{
			if(des_set_key(&(ctx->des[2]), k3, DES_DECRYPTION)){
				ret = -1;
				goto err;
			}
		}
	}
	else{
		ret = -1;
		goto err;
	}

	ret = 0;
err:
	return ret;
}

/* TDES encryption/decryption */
ATTRIBUTE_WARN_UNUSED_RET int des3(const des3_context *ctx, const unsigned char input[8], unsigned char output[8])
{
	int ret;
	unsigned char tmp[8];

	if(ctx == NULL){
		ret = -1;
		goto err;
	}
	if(ctx->dir == DES_ENCRYPTION){
		if(des_core(&(ctx->des[0]), input, output)){
			ret = -1;
			goto err;
		}
		if(des_core(&(ctx->des[1]), output, tmp)){
			ret = -1;
			goto err;
		}
		if(des_core(&(ctx->des[2]), tmp, output)){
			ret = -1;
			goto err;
		}
	}
	else if(ctx->dir == DES_DECRYPTION){
		if(des_core(&(ctx->des[2]), input, output)){
			ret = -1;
			goto err;
		}
		if(des_core(&(ctx->des[1]), output, tmp)){
			ret = -1;
			goto err;
		}
		if(des_core(&(ctx->des[0]), tmp, output)){
			ret = -1;
			goto err;
		}
	}
	else{
		ret = -1;
		goto err;
	}

	ret = 0;
err:
	return ret;
}
