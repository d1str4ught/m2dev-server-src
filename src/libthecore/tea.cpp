#include "stdafx.h"

#define TEA_ROUND		32
#define DELTA			0x9E3779B9

char tea_nilbuf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

INLINE void tea_code(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
{
    DWORD	y = sy, z = sz, sum = 0;

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 1
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 2
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 3
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 4
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 5
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 6
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 7
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 8
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 9
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 10
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 11
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 12
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 13
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 14
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 15
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 16
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 17
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 18
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 19
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 20
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 21
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 22
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 23
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 24
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 25
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 26
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 27
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 28
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 29
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 30
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 31
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    y	+= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);		// 32
    sum	+= DELTA;
    z	+= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);

    *(dest++)	= y;
    *dest	= z;
}

INLINE void tea_decode(const DWORD sz, const DWORD sy, const DWORD *key, DWORD *dest)
{
    DWORD y = sy, z = sz, sum = DELTA * TEA_ROUND;

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 1
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 2
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 3
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 4
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 5
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 6
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 7
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 8
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 9
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 10
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 11
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 12
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 13
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 14
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 15
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 16
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 17
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 18
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 19
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 20
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 21
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 22
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 23
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 24
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 25
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 26
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 27
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 28
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 29
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 30
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 31
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);		// 32
    sum -= DELTA;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);

    *(dest++)	= y;
    *dest	= z;
}

int TEA_Encrypt(DWORD *dest, const DWORD *src, const DWORD * key, int size)
{
    int		i;
    int		resize;

    if (size % 8 != 0)
    {
	resize = size + 8 - (size % 8);
	memset((char *) src + size, 0, resize - size);
    }
    else
	resize = size;

    for (i = 0; i < resize >> 3; i++, dest += 2, src += 2)
	tea_code(*(src + 1), *src, key, dest);

    return (resize);
}

int TEA_Decrypt(DWORD *dest, const DWORD *src, const DWORD * key, int size)
{
    int		i;
    int		resize;

    if (size % 8 != 0)
	resize = size + 8 - (size % 8);
    else
	resize = size;

    for (i = 0; i < resize >> 3; i++, dest += 2, src += 2)
	tea_decode(*(src + 1), *src, key, dest);

    return (resize);
}

