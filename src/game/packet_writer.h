#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>

// PacketWriter: Safe, bounds-checked packet serialization into a fixed buffer.
// Used to build outgoing packets before writing to the send buffer.
//
// Usage:
//   uint8_t buf[128];
//   PacketWriter w(buf, sizeof(buf));
//   w.WriteU16(CG::CHAT);
//   w.WriteU16(0); // placeholder for length
//   w.WriteU8(chatType);
//   w.WriteString(message, 128);
//   w.PatchU16(2, w.Written()); // patch length at offset 2
//   stream.Send(w.Written(), buf);

class PacketWriter
{
public:
	PacketWriter(void* buf, size_t capacity)
		: m_buf(static_cast<uint8_t*>(buf))
		, m_capacity(capacity)
		, m_pos(0)
	{
		assert(buf != nullptr);
		assert(capacity > 0);
	}

	// --- Write primitives ---

	bool WriteU8(uint8_t v)
	{
		if (m_pos + sizeof(v) > m_capacity)
			return false;
		m_buf[m_pos++] = v;
		return true;
	}

	bool WriteU16(uint16_t v)
	{
		if (m_pos + sizeof(v) > m_capacity)
			return false;
		std::memcpy(m_buf + m_pos, &v, sizeof(v));
		m_pos += sizeof(v);
		return true;
	}

	bool WriteU32(uint32_t v)
	{
		if (m_pos + sizeof(v) > m_capacity)
			return false;
		std::memcpy(m_buf + m_pos, &v, sizeof(v));
		m_pos += sizeof(v);
		return true;
	}

	bool WriteI8(int8_t v) { return WriteU8(static_cast<uint8_t>(v)); }
	bool WriteI16(int16_t v) { return WriteU16(static_cast<uint16_t>(v)); }
	bool WriteI32(int32_t v) { return WriteU32(static_cast<uint32_t>(v)); }

	bool WriteU64(uint64_t v)
	{
		if (m_pos + sizeof(v) > m_capacity)
			return false;
		std::memcpy(m_buf + m_pos, &v, sizeof(v));
		m_pos += sizeof(v);
		return true;
	}

	bool WriteFloat(float v)
	{
		if (m_pos + sizeof(v) > m_capacity)
			return false;
		std::memcpy(m_buf + m_pos, &v, sizeof(v));
		m_pos += sizeof(v);
		return true;
	}

	// Write raw bytes
	bool WriteBytes(const void* data, size_t len)
	{
		if (len == 0)
			return true;
		if (m_pos + len > m_capacity)
			return false;
		std::memcpy(m_buf + m_pos, data, len);
		m_pos += len;
		return true;
	}

	// Write a fixed-length null-padded string
	bool WriteString(const char* str, size_t fixedLen)
	{
		if (m_pos + fixedLen > m_capacity)
			return false;

		size_t srcLen = str ? std::strlen(str) : 0;
		size_t copyLen = (srcLen < fixedLen) ? srcLen : (fixedLen - 1);

		std::memcpy(m_buf + m_pos, str, copyLen);
		std::memset(m_buf + m_pos + copyLen, 0, fixedLen - copyLen);
		m_pos += fixedLen;
		return true;
	}

	// --- Patch: overwrite data at a previous offset ---

	bool PatchU16(size_t offset, uint16_t v)
	{
		if (offset + sizeof(v) > m_pos)
			return false;
		std::memcpy(m_buf + offset, &v, sizeof(v));
		return true;
	}

	bool PatchU32(size_t offset, uint32_t v)
	{
		if (offset + sizeof(v) > m_pos)
			return false;
		std::memcpy(m_buf + offset, &v, sizeof(v));
		return true;
	}

	// --- State ---

	size_t Written() const { return m_pos; }
	size_t Remaining() const { return m_capacity - m_pos; }
	const uint8_t* Data() const { return m_buf; }

	void Reset() { m_pos = 0; }

private:
	uint8_t* m_buf;
	size_t m_capacity;
	size_t m_pos;
};
