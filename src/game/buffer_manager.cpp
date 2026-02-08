#include "stdafx.h"
#include "buffer_manager.h"

TEMP_BUFFER::TEMP_BUFFER(int Size, bool bForceDelete)
{
	if (bForceDelete)
		Size = std::max(Size, 1024 * 128);

	m_buf.Reserve(static_cast<size_t>(Size));
}

const void * TEMP_BUFFER::read_peek()
{
	return m_buf.ReadPtr();
}

void TEMP_BUFFER::write(const void * data, int size)
{
	m_buf.Write(data, static_cast<size_t>(size));
}

int TEMP_BUFFER::size()
{
	return static_cast<int>(m_buf.ReadableBytes());
}

void TEMP_BUFFER::reset()
{
	m_buf.Clear();
}

void* TEMP_BUFFER::write_peek(int size)
{
	m_buf.EnsureWritable(static_cast<size_t>(size));
	return m_buf.WritePtr();
}

void TEMP_BUFFER::write_proceed(int size)
{
	m_buf.CommitWrite(static_cast<size_t>(size));
}
