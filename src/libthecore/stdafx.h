#pragma once

#if defined(__GNUC__)
#define INLINE __inline__
#elif defined(_MSC_VER)
#define INLINE inline
#endif

#ifdef OS_WINDOWS
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <conio.h>
#include <process.h>
#include <limits.h>
#include <math.h>
#include <locale.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>

#include "xdirent.h"
#include "xgetopt.h"

#define S_ISDIR(m)	(m & _S_IFDIR)
#define snprintf _snprintf

#define __USE_SELECT__

#define PATH_MAX _MAX_PATH

// C runtime library adjustments

#define strcasecmp(s1, s2) stricmp(s1, s2)
#define strncasecmp(s1, s2, n) strnicmp(s1, s2, n)
#define strtok_r(s, delim, ptrptr) strtok_s(s, delim, ptrptr)
#define strlcat(dst, src, size) strcat_s(dst, size, src)
#define localtime_r(timet, result) localtime_s(result, timet)
#define strlcpy(dst, src, size) strncpy_s(dst, size, src, _TRUNCATE)

#define __typeof(t) auto

// dummy declaration of non-supported signals
#define SIGUSR1     30  /* user defined signal 1 */
#define SIGUSR2     31  /* user defined signal 2 */

inline void usleep(unsigned long usec) {
	::Sleep(usec / 1000);
}
inline unsigned sleep(unsigned sec) {
	::Sleep(sec * 1000);
	return 0;
}
inline double rint(double x)
{
	return ::floor(x+.5);
}


#else

#ifndef OS_FREEBSD
#define __USE_SELECT__
#ifdef __CYGWIN__
#define _POSIX_SOURCE 1
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <sys/signal.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>

#ifdef OS_FREEBSD
#include <sys/event.h>
#endif

#endif

#ifndef FALSE
#define FALSE	false
#define TRUE	true
#endif

#include "typedef.h"
#include "heart.h"
#include "fdwatch.h"
#include "socket.h"
#include "kstbl.h"
#include "hangul.h"

#include "signal.h"
#include "log.h"
#include "main.h"
#include "utils.h"
#include "memcpy.h"
