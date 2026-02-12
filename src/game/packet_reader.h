#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>

// PacketReader: Safe, bounds-checked packet deserialization from a read-only buffer.
// Used to parse incoming packets after the dispatch loop validates the header and length.
//
// Usage:
//   PacketReader r(packetData, packetLength);
//   uint16_t header = r.ReadU16();
//   uint16_t length = r.ReadU16();
//   uint8_t chatType = r.ReadU8();
//   char message[128];
//   r.ReadString(message, sizeof(message));

class PacketReader
{
public:
	PacketReader(const void* data, size_t size)
		: m_buf(static_cast<const uint8_t*>(data))
		, m_size(size)
		, m_pos(0)
	{
		assert(data != nullptr || size == 0);
	}

	// --- Read primitives ---

	uint8_t ReadU8()
	{
		assert(m_pos + sizeof(uint8_t) <= m_size);
		return m_buf[m_pos++];
	}

	uint16_t ReadU16()
	{
		assert(m_pos + sizeof(uint16_t) <= m_size);
		uint16_t v;
		std::memcpy(&v, m_buf + m_pos, sizeof(v));
		m_pos += sizeof(v);
		return v;
	}

	uint32_t ReadU32()
	{
		assert(m_pos + sizeof(uint32_t) <= m_size);
		uint32_t v;
		std::memcpy(&v, m_buf + m_pos, sizeof(v));
		m_pos += sizeof(v);
		return v;
	}

	int8_t ReadI8() { return static_cast<int8_t>(ReadU8()); }
	int16_t ReadI16() { return static_cast<int16_t>(ReadU16()); }
	int32_t ReadI32() { return static_cast<int32_t>(ReadU32()); }

	uint64_t ReadU64()
	{
		assert(m_pos + sizeof(uint64_t) <= m_size);
		uint64_t v;
		std::memcpy(&v, m_buf + m_pos, sizeof(v));
		m_pos += sizeof(v);
		return v;
	}

	float ReadFloat()
	{
		assert(m_pos + sizeof(float) <= m_size);
		float v;
		std::memcpy(&v, m_buf + m_pos, sizeof(v));
		m_pos += sizeof(v);
		return v;
	}

	// Read raw bytes
	bool ReadBytes(void* dest, size_t len)
	{
		if (m_pos + len > m_size)
			return false;
		std::memcpy(dest, m_buf + m_pos, len);
		m_pos += len;
		return true;
	}

	// Read a fixed-length string (null-terminated in dest)
	bool ReadString(char* dest, size_t fixedLen)
	{
		if (m_pos + fixedLen > m_size)
			return false;
		std::memcpy(dest, m_buf + m_pos, fixedLen);
		dest[fixedLen - 1] = '\0'; // ensure null termination
		m_pos += fixedLen;
		return true;
	}

	// Skip bytes without reading
	bool Skip(size_t len)
	{
		if (m_pos + len > m_size)
			return false;
		m_pos += len;
		return true;
	}

	// --- Peek (non-consuming) ---

	uint8_t PeekU8() const
	{
		assert(m_pos + sizeof(uint8_t) <= m_size);
		return m_buf[m_pos];
	}

	uint16_t PeekU16() const
	{
		assert(m_pos + sizeof(uint16_t) <= m_size);
		uint16_t v;
		std::memcpy(&v, m_buf + m_pos, sizeof(v));
		return v;
	}

	uint16_t PeekU16At(size_t offset) const
	{
		assert(offset + sizeof(uint16_t) <= m_size);
		uint16_t v;
		std::memcpy(&v, m_buf + offset, sizeof(v));
		return v;
	}

	template<typename T>
	bool ReadStruct(T& out)
	{
		if (m_pos + sizeof(T) > m_size)
			return false;
		std::memcpy(&out, m_buf + m_pos, sizeof(T));
		m_pos += sizeof(T);
		return true;
	}

	// --- State ---

	bool HasBytes(size_t n) const { return (m_pos + n) <= m_size; }
	size_t Remaining() const { return m_size - m_pos; }
	size_t Position() const { return m_pos; }
	size_t Size() const { return m_size; }
	const uint8_t* Current() const { return m_buf + m_pos; }

	void Reset() { m_pos = 0; }

private:
	const uint8_t* m_buf;
	size_t m_size;
	size_t m_pos;
};
