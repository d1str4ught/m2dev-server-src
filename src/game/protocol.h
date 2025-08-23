#pragma once
#include <cassert>
#include <cstdint>
#include "libthecore/buffer.h"

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

#define packet_encode(buf, data, len) __packet_encode(buf, data, len, __FILE__, __LINE__)

//#define DEFAULT_PACKET_BUFFER_SIZE 32768
#define DEFAULT_PACKET_BUFFER_SIZE 65536

inline bool __packet_encode(LPBUFFER& pbuf, const void * data, int length, const char * file, int line)
{
	assert(NULL != pbuf);
	assert(NULL != data);

	if (buffer_has_space(pbuf) < length)
	{
		//sys_err("buffer length exceeded buffer size: %d, encoding %d bytes (%s:%d)", buffer_size(pbuf), length, file, line);
		return false;
	}

	//buffer_adjust_size(pbuf, length);
	buffer_write(pbuf, data, length);
	return true;
}
