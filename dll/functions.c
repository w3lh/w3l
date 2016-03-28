#if !defined(USE_SRP3)

#include "functions.h"

/*
Password hashing functions here are adapted from bnethash.cpp (part of pvpgn):
https://svn.berlios.de/wsvn/pvpgn/trunk/pvpgn/src/common/bnethash.cpp?rev=257&sc=0
*/

/*
 * Fill 16 elements of the array of 32 bit values with the bytes from
 * dst up to count in little endian order. Fill left over space with
 * zeros
 */
static void hash_set_16(uint32_t * dst, unsigned char const * src, unsigned int count)
{
    unsigned int i;
    unsigned int pos;

    for (pos=0,i=0; i<16; i++)
    {
        dst[i] = 0;
        if (pos<count)
            dst[i] |= ((uint32_t)src[pos]);
        pos++;
        if (pos<count)
            dst[i] |= ((uint32_t)src[pos])<<8;
        pos++;
        if (pos<count)
            dst[i] |= ((uint32_t)src[pos])<<16;
        pos++;
        if (pos<count)
            dst[i] |= ((uint32_t)src[pos])<<24;
        pos++;
    }
}

DLLEXPORT void w3l_do_hash(char *username, bnet_hash_ctx *ctx) {
    int i;
    uint32_t     a,b,c,d,e,g;
	int32_t      tmp[16+64];
	char        *password;

	password = (char *)(ctx+1);
	hash_set_16(tmp, password, strlen(password));

	for (i=0; i<64; i++)
		tmp[i+16] = ROTL32(1,tmp[i] ^ tmp[i+8] ^ tmp[i+2] ^ tmp[i+13]);

	a = ctx->a;
	b = ctx->b;
	c = ctx->c;
	d = ctx->d;
	e = ctx->e;

    for (i=0; i<20*1; i++)
    {
	g = tmp[i] + ROTL32(a,5) + e + ((b & c) | (~b & d)) + 0x5a827999;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }

    for (; i<20*2; i++)
    {
	g = (d ^ c ^ b) + e + ROTL32(g,5) + tmp[i] + 0x6ed9eba1;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }

    for (; i<20*3; i++)
    {
	g = tmp[i] + ROTL32(g,5) + e + ((c & b) | (d & c) | (d & b)) - 0x70e44324;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }

    for (; i<20*4; i++)
    {
	g = (d ^ c ^ b) + e + ROTL32(g,5) + tmp[i] - 0x359d3e2a;
	e = d;
	d = c;
	c = ROTL32(b,30);
	b = a;
	a = g;
    }

	ctx->a += g;
	ctx->b += b;
	ctx->c += c;
	ctx->d += d;
	ctx->e += e;
}

DLLEXPORT void __fastcall w3l_hash_init(uint32_t *data) {
	*(data+0) = 0x67452301;
	*(data+1) = 0xEFCDAB89;
	*(data+2) = 0x98BADCFE;
	*(data+3) = 0x10325476;
	*(data+4) = 0xC3D2E1F0;
	*(data+5) = 0;
	*(data+6) = 0;
}

/* by Rupan, 8/23/2008 -- reserved for future use
   this implements logon_proof_hash
   Not sure about the argument types here.....
*/
DLLEXPORT signed int w3l_logon_proof_hash(char *arg1, char *arg2, char *arg3) {
	int i;
	char buf[5*4+64], *tmp;

	tmp = buf + (5*4);
	w3l_hash_init((uint32_t *)buf);
	memset(tmp, 0, 64);
	memcpy(tmp, (const void *)(arg1 + 32), 16);

	for(i=0; i<16; i++) {
		if(tmp[i] >= 65 && tmp[i] <= 90) {
				tmp[i] |= 0x20;
		}
	}

	w3l_do_hash(arg1, (bnet_hash_ctx *)buf);
	memcpy((arg1+168), (const void *)buf, 20);

	return 1;
}

/*
 * This function wraps logon_proof_hash.  It can be found at address
 * 0x6F6B2870 in Game.dll v1.24.
 * Fairly sure about the argument counts herein.
*/
DLLEXPORT int __fastcall w3l_lph_checked(int *a1, int *a2, void *a3, void *a4)
{
  int result;

  result = w3l_logon_proof_hash(a2, a3, a4);
  if ( result )
  {
    *(a1 + 0) = *(a2 + 42); /* a2 + 168 bytes */
    *(a1 + 1) = *(a2 + 43); /* a2 + 172 bytes */
    *(a1 + 2) = *(a2 + 44); /* a2 + 176 bytes */
    *(a1 + 3) = *(a2 + 45); /* a2 + 180 bytes */
    *(a1 + 4) = *(a2 + 46); /* a2 + 184 bytes */
  }
  return result;
}

#endif
