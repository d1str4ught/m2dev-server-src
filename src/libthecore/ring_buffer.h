#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>

class RingBuffer
{
public:
	explicit RingBuffer(size_t initialCapacity = 0)
	{
		if (initialCapacity > 0)
			m_data.resize(initialCapacity);
	}

	~RingBuffer() = default;

	// Non-copyable, movable
	RingBuffer(const RingBuffer&) = delete;
	RingBuffer& operator=(const RingBuffer&) = delete;
	RingBuffer(RingBuffer&&) noexcept = default;
	RingBuffer& operator=(RingBuffer&&) noexcept = default;

	// --- Capacity ---

	size_t ReadableBytes() const { return m_writePos - m_readPos; }
	size_t WritableBytes() const { return m_data.size() - m_writePos; }
	size_t Capacity() const { return m_data.size(); }

	void Reserve(size_t capacity)
	{
		if (capacity > m_data.size())
		{
			Compact();
			m_data.resize(capacity);
		}
	}

	void EnsureWritable(size_t len)
	{
		if (WritableBytes() >= len)
			return;

		// Try compaction first
		Compact();
		if (WritableBytes() >= len)
			return;

		// Must grow
		size_t needed = m_writePos + len;
		size_t newSize = m_data.size();
		if (newSize == 0)
			newSize = 1024;
		while (newSize < needed)
			newSize *= 2;

		m_data.resize(newSize);
	}

	// --- Write operations ---

	void Write(const void* data, size_t len)
	{
		if (len == 0)
			return;

		assert(data != nullptr);
		EnsureWritable(len);
		std::memcpy(m_data.data() + m_writePos, data, len);
		m_writePos += len;
	}

	// Direct write access for socket recv()
	uint8_t* WritePtr()
	{
		return m_data.data() + m_writePos;
	}

	void CommitWrite(size_t len)
	{
		assert(m_writePos + len <= m_data.size());
		m_writePos += len;
	}

	// --- Read operations ---

	bool HasBytes(size_t n) const { return ReadableBytes() >= n; }

	bool Peek(void* dest, size_t len) const
	{
		if (ReadableBytes() < len)
			return false;

		std::memcpy(dest, m_data.data() + m_readPos, len);
		return true;
	}

	// Peek at a specific offset from read position (non-consuming)
	bool PeekAt(size_t offset, void* dest, size_t len) const
	{
		if (ReadableBytes() < offset + len)
			return false;

		std::memcpy(dest, m_data.data() + m_readPos + offset, len);
		return true;
	}

	bool Read(void* dest, size_t len)
	{
		if (!Peek(dest, len))
			return false;

		m_readPos += len;
		MaybeCompact();
		return true;
	}

	// Discard bytes without reading them
	bool Discard(size_t len)
	{
		if (ReadableBytes() < len)
			return false;

		m_readPos += len;
		MaybeCompact();
		return true;
	}

	// Direct read access for socket send() and packet processing
	const uint8_t* ReadPtr() const
	{
		return m_data.data() + m_readPos;
	}

	size_t ReadPos() const { return m_readPos; }
	size_t WritePos() const { return m_writePos; }

	// --- Encryption support ---
	// Get writable pointer to already-written data at a specific position
	// Used for in-place encryption of data that was just written
	uint8_t* DataAt(size_t pos)
	{
		assert(pos < m_data.size());
		return m_data.data() + pos;
	}

	// --- Reset ---

	void Clear()
	{
		m_readPos = 0;
		m_writePos = 0;
	}

	// --- Compaction ---

	void Compact()
	{
		if (m_readPos == 0)
			return;

		size_t readable = ReadableBytes();
		if (readable > 0)
			std::memmove(m_data.data(), m_data.data() + m_readPos, readable);

		m_readPos = 0;
		m_writePos = readable;
	}

private:
	void MaybeCompact()
	{
		// Compact when read position passes halfway and there's no readable data
		// or when read position is more than half the buffer
		if (m_readPos == m_writePos)
		{
			m_readPos = 0;
			m_writePos = 0;
		}
		else if (m_readPos > m_data.size() / 2)
		{
			Compact();
		}
	}

	std::vector<uint8_t> m_data;
	size_t m_readPos = 0;
	size_t m_writePos = 0;
};
