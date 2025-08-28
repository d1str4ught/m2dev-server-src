#ifndef __INC_CRC32_H__
#define __INC_CRC32_H__

#ifndef OS_WINDOWS
#include <unistd.h>
#endif

typedef unsigned long crc_t;

crc_t	get_crc32(const char * buffer, size_t count);
crc_t	get_crc32_case(const char * buffer, size_t count);
crc_t	get_fast_hash(const char * key, size_t len);

#define CRC32(buf) get_crc32(buf, strlen(buf))
#define CRC32CASE(buf) get_crc32_case(buf, strlen(buf))

#endif
