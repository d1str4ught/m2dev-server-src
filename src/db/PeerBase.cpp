#include "stdafx.h"
#include "PeerBase.h"

CPeerBase::CPeerBase() : m_fd(INVALID_SOCKET), m_BytesRemain(0)
{
}

CPeerBase::~CPeerBase()
{
	Destroy();
}

void CPeerBase::Disconnect()
{
	if (m_fd != INVALID_SOCKET)
	{
		fdwatch_del_fd(m_fdWatcher, m_fd);

		socket_close(m_fd);
		m_fd = INVALID_SOCKET;
	}
}

void CPeerBase::Destroy()
{
	Disconnect();
	m_outBuffer.Clear();
	m_inBuffer.Clear();
}

bool CPeerBase::Accept(socket_t fd_accept)
{
	struct sockaddr_in peer;

	if ((m_fd = socket_accept(fd_accept, &peer)) == INVALID_SOCKET)
	{
		Destroy();
		return false;
	}

	//socket_block(m_fd);
	socket_sndbuf(m_fd, 233016);
	socket_rcvbuf(m_fd, 233016);

	strlcpy(m_host, inet_ntoa(peer.sin_addr), sizeof(m_host));
	m_outBuffer.Reserve(DEFAULT_PACKET_BUFFER_SIZE);
	m_inBuffer.Reserve(MAX_INPUT_LEN);

	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_READ, false);

	OnAccept();
	sys_log(0, "ACCEPT FROM %s", inet_ntoa(peer.sin_addr));
	return true;
}

bool CPeerBase::Connect(const char* host, WORD port)
{
	strlcpy(m_host, host, sizeof(m_host));

	if ((m_fd = socket_connect(host, port)) == INVALID_SOCKET)
		return false;

	m_outBuffer.Reserve(DEFAULT_PACKET_BUFFER_SIZE);

	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_READ, false);

	OnConnect();
	return true;
}

void CPeerBase::Close()
{
	OnClose();
}

void CPeerBase::EncodeBYTE(uint8_t b)
{
	m_outBuffer.Write(&b, sizeof(uint8_t));
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::EncodeWORD(uint16_t w)
{
	m_outBuffer.Write(&w, sizeof(uint16_t));
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::EncodeDWORD(uint32_t dw)
{
	m_outBuffer.Write(&dw, sizeof(uint32_t));
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

void CPeerBase::Encode(const void* data, uint32_t size)
{
	m_outBuffer.Write(data, size);
	fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
}

int CPeerBase::Recv()
{
	m_inBuffer.EnsureWritable(MAX_INPUT_LEN >> 2);
	int bytes_to_read = static_cast<int>(m_inBuffer.WritableBytes());
	ssize_t bytes_read = socket_read(m_fd, (char *) m_inBuffer.WritePtr(), bytes_to_read);

	if (bytes_read < 0)
	{
		sys_err("socket_read failed %s", strerror(errno));
		return -1;
	}
	else if (bytes_read == 0)
		return 0;

	m_inBuffer.CommitWrite(bytes_read);
	m_BytesRemain = static_cast<int>(m_inBuffer.ReadableBytes());
	return 1;
}

void CPeerBase::RecvEnd(int proceed_bytes)
{
	m_inBuffer.Discard(proceed_bytes);
	m_BytesRemain = static_cast<int>(m_inBuffer.ReadableBytes());
}

int CPeerBase::GetRecvLength()
{
	return m_BytesRemain;
}

const void * CPeerBase::GetRecvBuffer()
{
	return m_inBuffer.ReadPtr();
}

int CPeerBase::GetSendLength()
{
	return static_cast<int>(m_outBuffer.ReadableBytes());
}

int CPeerBase::Send()
{
	if (m_outBuffer.ReadableBytes() <= 0)
		return 0;

	int iBufferLeft = fdwatch_get_buffer_size(m_fdWatcher, m_fd);
	int iBytesToWrite = MIN(iBufferLeft, static_cast<int>(m_outBuffer.ReadableBytes()));

	if (iBytesToWrite == 0)
		return 0;

	int result = socket_write(m_fd, (const char *) m_outBuffer.ReadPtr(), iBytesToWrite);

	if (result == 0)
	{
		m_outBuffer.Discard(iBytesToWrite);

		if (m_outBuffer.ReadableBytes() != 0)
			fdwatch_add_fd(m_fdWatcher, m_fd, this, FDW_WRITE, true);
	}

	return (result);
}
