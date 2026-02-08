#ifndef __INC_METIN_II_GAME_BUFFER_MANAGER_H__
#define __INC_METIN_II_GAME_BUFFER_MANAGER_H__

#include "libthecore/ring_buffer.h"

class TEMP_BUFFER
{
	public:
		TEMP_BUFFER(int Size = 8192, bool ForceDelete = false);
		~TEMP_BUFFER() = default;

		const void * 	read_peek();
		void		write(const void * data, int size);
		int		size();
		void	reset();

		// Direct write access for building packets with deferred headers
		void*		write_peek(int size);
		void		write_proceed(int size);

	protected:
		RingBuffer	m_buf;
};

#endif
