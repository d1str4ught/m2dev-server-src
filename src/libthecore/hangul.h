#pragma once

#ifdef OS_WINDOWS
#define isdigit iswdigit
#define isspace iswspace
#endif

#define ishan(ch)       (((ch) & 0xE0) > 0x90)
#define ishanasc(ch)    (isascii(ch) || ishan(ch))
#define ishanalp(ch)    (isalpha(ch) || ishan(ch))
#define isnhdigit(ch)   (!ishan(ch) && isdigit(ch))
#define isnhspace(ch)   (!ishan(ch) && isspace(ch))

const char *	first_han(const BYTE * str);
int				check_han(const char * str);
int				is_hangul(const BYTE * str);
int				under_han(const void * orig);

#define UNDER(str)	under_han(str)
