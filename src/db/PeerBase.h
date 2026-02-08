#pragma once

#include "NetBase.h"
#include "libthecore/ring_buffer.h"

class CPeerBase : public CNetBase
{
    public:
	enum
	{
	    MAX_HOST_LENGTH		= 30,
	    MAX_INPUT_LEN		= 1024 * 1024 * 2,
	    DEFAULT_PACKET_BUFFER_SIZE	= 1024 * 1024 * 2
	};

    protected:
	virtual void	OnAccept() = 0;
	virtual void	OnConnect() = 0;
	virtual void	OnClose() = 0;

    public:
	bool		Accept(socket_t accept_fd);
	bool		Connect(const char* host, WORD port);
	void		Close();

    public:
	CPeerBase();
	virtual ~CPeerBase();

	void		Disconnect();
	void		Destroy();

	socket_t	GetFd() { return m_fd; }

	void		EncodeBYTE(uint8_t b);
	void		EncodeWORD(uint16_t w);
	void		EncodeDWORD(uint32_t dw);
	void		Encode(const void* data, uint32_t size);
	int		Send();

	int		Recv();
	void		RecvEnd(int proceed_bytes);
	int		GetRecvLength();
	const void *	GetRecvBuffer();

	int		GetSendLength();

	const char *	GetHost() { return m_host; }

    protected:
	char		m_host[MAX_HOST_LENGTH + 1];
	socket_t	m_fd;

    private:
	int		m_BytesRemain;
	RingBuffer	m_outBuffer;
	RingBuffer	m_inBuffer;
};
