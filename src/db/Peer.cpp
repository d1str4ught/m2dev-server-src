#include "stdafx.h"
#include "Peer.h"
#include "ItemIDRangeManager.h"

CPeer::CPeer()
{
	m_state = 0;
	m_bChannel = 0;
	m_dwHandle = 0;
	m_dwUserCount = 0;
	m_wListenPort = 0;
	m_wP2PPort = 0;

	memset(m_alMaps, 0, sizeof(m_alMaps));

	m_itemRange.dwMin = m_itemRange.dwMax = m_itemRange.dwUsableItemIDMin = 0;
	m_itemSpareRange.dwMin = m_itemSpareRange.dwMax = m_itemSpareRange.dwUsableItemIDMin = 0;
}

CPeer::~CPeer()
{
	Close();
}

void CPeer::OnAccept()
{
	m_state = STATE_PLAYING;

	static DWORD current_handle = 0;
	m_dwHandle = ++current_handle;

	sys_log(0, "Connection accepted. (host: %s handle: %u fd: %d)", m_host, m_dwHandle, m_fd);
}

void CPeer::OnConnect()
{
	sys_log(0, "Connection established. (host: %s handle: %u fd: %d)", m_host, m_dwHandle, m_fd);
	m_state = STATE_PLAYING;
}

void CPeer::OnClose()
{
	m_state = STATE_CLOSE;

	sys_log(0, "Connection closed. (host: %s)", m_host);
	sys_log(0, "ItemIDRange: returned. %u ~ %u", m_itemRange.dwMin, m_itemRange.dwMax);

	CItemIDRangeManager::instance().UpdateRange(m_itemRange.dwMin, m_itemRange.dwMax);

	m_itemRange.dwMin = 0;
	m_itemRange.dwMax = 0;
	m_itemRange.dwUsableItemIDMin = 0;
}

DWORD CPeer::GetHandle()
{
	return m_dwHandle;
}

DWORD CPeer::GetUserCount()
{
	return m_dwUserCount;
}

void CPeer::SetUserCount(DWORD dwCount)
{
	m_dwUserCount = dwCount;
}

bool CPeer::PeekPacket(int & iBytesProceed, uint16_t & wHeader, DWORD & dwHandle, DWORD & dwLength, const char ** data)
{
	static constexpr int FRAME_SIZE = sizeof(HEADER); // 10: header(2) + handle(4) + size(4)

	if (GetRecvLength() < iBytesProceed + FRAME_SIZE)
		return false;

	const char * buf = (const char *) GetRecvBuffer();
	buf += iBytesProceed;

	wHeader		= *((uint16_t *) buf);
	buf		+= sizeof(uint16_t);

	dwHandle	= *((DWORD *) buf);
	buf		+= sizeof(DWORD);

	dwLength	= *((DWORD *) buf);
	buf		+= sizeof(DWORD);

	//sys_log(0, "%d header %d handle %u length %u", GetRecvLength(), wHeader, dwHandle, dwLength);
	if (iBytesProceed + dwLength + FRAME_SIZE > (DWORD) GetRecvLength())
	{
		sys_log(0, "PeekPacket: not enough buffer size: len %u, recv %d",
				FRAME_SIZE+dwLength, GetRecvLength()-iBytesProceed);
		return false;
	}

	*data = buf;
	iBytesProceed += dwLength + FRAME_SIZE;
	return true;
}

void CPeer::EncodeHeader(uint16_t wHeader, DWORD dwHandle, DWORD dwSize)
{
	HEADER h;

	sys_log(1, "EncodeHeader %u handle %u size %u", wHeader, dwHandle, dwSize);

	h.wHeader = wHeader;
	h.dwHandle = dwHandle;
	h.dwSize = dwSize;

	Encode(&h, sizeof(HEADER));
}

void CPeer::EncodeReturn(uint16_t wHeader, DWORD dwHandle)
{
	EncodeHeader(wHeader, dwHandle, 0);
}

int CPeer::Send()
{
	if (m_state == STATE_CLOSE)
		return -1;

	return (CPeerBase::Send());
}

void CPeer::SetP2PPort(WORD wPort)
{
	m_wP2PPort = wPort;
}

void CPeer::SetMaps(int32_t * pl)
{
	thecore_memcpy(m_alMaps, pl, sizeof(m_alMaps));
}

void CPeer::SendSpareItemIDRange()
{
	if (m_itemSpareRange.dwMin == 0 || m_itemSpareRange.dwMax == 0 || m_itemSpareRange.dwUsableItemIDMin == 0)
	{
		EncodeHeader(DG::ACK_SPARE_ITEM_ID_RANGE, 0, sizeof(TItemIDRangeTable));
		Encode(&m_itemSpareRange, sizeof(TItemIDRangeTable));
	}
	else
	{
		SetItemIDRange(m_itemSpareRange);

		if (SetSpareItemIDRange(CItemIDRangeManager::instance().GetRange()) == false)
		{
			sys_log(0, "ItemIDRange: spare range set error");
			m_itemSpareRange.dwMin = m_itemSpareRange.dwMax = m_itemSpareRange.dwUsableItemIDMin = 0;
		}

		EncodeHeader(DG::ACK_SPARE_ITEM_ID_RANGE, 0, sizeof(TItemIDRangeTable));
		Encode(&m_itemSpareRange, sizeof(TItemIDRangeTable));
	}
}

bool CPeer::SetItemIDRange(TItemIDRangeTable itemRange)
{
	if (itemRange.dwMin == 0 || itemRange.dwMax == 0 || itemRange.dwUsableItemIDMin == 0) return false;

	m_itemRange = itemRange;
	sys_log(0, "ItemIDRange: SET %s %u ~ %u start: %u", GetPublicIP(), m_itemRange.dwMin, m_itemRange.dwMax, m_itemRange.dwUsableItemIDMin);

	return true;
}

bool CPeer::SetSpareItemIDRange(TItemIDRangeTable itemRange)
{
	if (itemRange.dwMin == 0 || itemRange.dwMax == 0 || itemRange.dwUsableItemIDMin == 0) return false;

	m_itemSpareRange = itemRange;
	sys_log(0, "ItemIDRange: SPARE SET %s %u ~ %u start: %u", GetPublicIP(), m_itemSpareRange.dwMin, m_itemSpareRange.dwMax,
			m_itemSpareRange.dwUsableItemIDMin);

	return true;
}

bool CPeer::CheckItemIDRangeCollision(TItemIDRangeTable itemRange)
{
	if (m_itemRange.dwMin < itemRange.dwMax && m_itemRange.dwMax > itemRange.dwMin)
	{
		sys_err("ItemIDRange: Collision!! this %u ~ %u check %u ~ %u",
				m_itemRange.dwMin, m_itemRange.dwMax, itemRange.dwMin, itemRange.dwMax);
		return false;
	}

	if (m_itemSpareRange.dwMin < itemRange.dwMax && m_itemSpareRange.dwMax > itemRange.dwMin)
	{
		sys_err("ItemIDRange: Collision with spare range this %u ~ %u check %u ~ %u",
				m_itemSpareRange.dwMin, m_itemSpareRange.dwMax, itemRange.dwMin, itemRange.dwMax);
		return false;
	}
	
	return true;
}


