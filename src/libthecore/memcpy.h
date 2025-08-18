#pragma once

void thecore_find_best_memcpy();

#ifndef OS_WINDOWS
    extern void *(*thecore_memcpy) (void * to, const void * from, size_t len);
#else
#include <cstring>
#define thecore_memcpy memcpy
#endif
