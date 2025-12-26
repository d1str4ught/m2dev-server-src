#pragma once

#define SAFE_FREE(p)		{ if (p) { free( (void *) p);		(p) = NULL;  } }
#define SAFE_DELETE(p)		{ if (p) { delete (p);			(p) = NULL;  } }
#define SAFE_DELETE_ARRAY(p)	{ if (p) { delete[] (p);		(p) = NULL;  } }
#define SAFE_RELEASE(p)		{ if (p) { (p)->Release();		(p) = NULL;  } }

#define LOWER(c)	(((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)	(((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c))

#define str_cmp strcasecmp
#define STRNCPY(dst, src, len)          do {strncpy(dst, src, len); dst[len] = '\0'; } while(0)

 char *	str_dup(const char * source);
 void		printdata(const unsigned char * data, int bytes);
 int		filesize(FILE * fp);

#define core_dump()	core_dump_unix(__FILE__, __LINE__)
void		core_dump_unix(const char *who, WORD line);

#define TOKEN(string) if (!str_cmp(token_string, string))
void		parse_token(char * src, char * token, char * value);

void		trim_and_lower(const char * src, char * dest, size_t dest_size);

void		lower_string(const char * src, char * dest, size_t dest_len);

int		is_abbrev(char *arg1, char *arg2);

struct timeval *	timediff(const struct timeval *a, const struct timeval *b);

struct timeval *	timeadd(struct timeval *a, struct timeval *b);

struct tm *		tm_calc(const struct tm *curr_tm, int days);

int MAX(int a, int b);
int MIN(int a, int b);
int MINMAX(int min, int value, int max);

int		number_ex(int from, int to, const char *file, int line);
#define number(from, to) number_ex(from, to, __FILE__, __LINE__)

float	fnumber(float from, float to);
extern float gauss_random(float avg = 0, float sigma = 1);

void		thecore_sleep(struct timeval * timeout);

float	get_float_time();
DWORD	get_dword_time();

char *	time_str(time_t ct);

#define CREATE(result, type, number)  do { \
	if (!((result) = (type *) calloc ((number), sizeof(type)))) { \
		sys_err("calloc failed [%d] %s", errno, strerror(errno)); \
		abort(); } } while(0)

#define RECREATE(result,type,number) do { \
	if (!((result) = (type *) realloc ((result), sizeof(type) * (number)))) { \
		sys_err("realloc failed [%d] %s", errno, strerror(errno)); \
		abort(); } } while(0)

#define INSERT_TO_TW_LIST(item, head, prev, next)   \
    if (!(head))                                    \
    {                                               \
	head         = item;                        \
	    (head)->prev = (head)->next = NULL;         \
    }                                               \
    else                                            \
    {                                               \
	(head)->prev = item;                        \
	    (item)->next = head;                    \
	    (item)->prev = NULL;                    \
	    head         = item;                    \
    }

#define REMOVE_FROM_TW_LIST(item, head, prev, next)	\
    if ((item) == (head))           			\
    {                               			\
	if (((head) = (item)->next))			\
	    (head)->prev = NULL;    			\
    }                    				\
    else                 				\
    {                    				\
	if ((item)->next)				\
	    (item)->next->prev = (item)->prev;		\
							\
	if ((item)->prev)				\
	    (item)->prev->next = (item)->next;		\
    }


#define INSERT_TO_LIST(item, head, next)            \
    (item)->next = (head);                      \
	(head) = (item);                            \

#define REMOVE_FROM_LIST(item, head, next)          \
	if ((item) == (head))                       \
	    head = (item)->next;                     \
	else                                        \
	{                                           \
	    temp = head;                            \
		\
		while (temp && (temp->next != (item)))  \
		    temp = temp->next;                  \
			\
			if (temp)                               \
			    temp->next = (item)->next;          \
	}                                           \

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                      \
                ((DWORD)(BYTE) (ch0       ) | ((DWORD)(BYTE) (ch1) <<  8) | \
                 ((DWORD)(BYTE) (ch2) << 16) | ((DWORD)(BYTE) (ch3) << 24))
#endif // defined(MAKEFOURCC)

// _countof for gcc/g++
#if !defined(_countof)
#if !defined(__cplusplus)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++"
{
	    template <typename _CountofType, size_t _SizeOfArray>
			        char (*__countof_helper(_CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) sizeof(*__countof_helper(_Array))
}
#endif
#endif

#ifdef OS_WINDOWS
extern void gettimeofday(struct timeval* t, struct timezone* dummy);
#endif

