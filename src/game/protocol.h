#pragma once
#include <cassert>
#include <cstdint>

inline const char *encode_byte(uint8_t ind)
{
	static char a[1];
	*((uint8_t*)a) = ind;
	return (a);
}

inline const char *encode_2bytes(uint16_t ind)
{
	static char a[2];
	*((uint16_t*)a) = ind;
	return (a);
}

inline const char *encode_4bytes(uint32_t ind)
{
	static char a[4];
	*((uint32_t*)a) = ind;
	return (a);
}

inline uint8_t decode_byte(const void * a)
{
	return (*(uint8_t*)a);
}

inline uint16_t decode_2bytes(const void * a)
{
	return (*((uint16_t*)a));
}

inline uint32_t decode_4bytes(const void *a)
{
	return (*((uint32_t*)a));
}
