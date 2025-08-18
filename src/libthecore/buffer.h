#pragma once

#define SAFE_BUFFER_DELETE(buf)		{ if(buf != NULL) { buffer_delete(buf); buf = NULL; } }

typedef struct buffer	BUFFER;
typedef struct buffer *	LPBUFFER;

struct buffer
{
	struct buffer * next;

	char *          write_point;
	int             write_point_pos;

	const char *    read_point;
	int             length;

	char *          mem_data;
	int             mem_size;

	long            flag;
};

LPBUFFER	buffer_new(int size);
void		buffer_delete(LPBUFFER buffer);
void		buffer_reset(LPBUFFER buffer);

DWORD		buffer_size(LPBUFFER buffer);
int			buffer_has_space(LPBUFFER buffer);

void		buffer_write (LPBUFFER& buffer, const void* src, int length);
void		buffer_read(LPBUFFER buffer, void * buf, int bytes);
BYTE		buffer_get_byte(LPBUFFER buffer);
WORD		buffer_get_word(LPBUFFER buffer);
DWORD		buffer_get_dword(LPBUFFER buffer);

const void* buffer_read_peek(LPBUFFER buffer);
void		buffer_read_proceed(LPBUFFER buffer, int length);

void*		buffer_write_peek(LPBUFFER buffer);
void		buffer_write_proceed(LPBUFFER buffer, int length);

void		buffer_adjust_size(LPBUFFER& buffer, int add_size);
