#include "stdafx.h"
#include "Lock.h"

CLock::CLock()
{
}

CLock::~CLock()
{
}

void CLock::Initialize()
{
	m_bLocked = false;
#ifndef OS_WINDOWS
	pthread_mutex_init(&m_lock, NULL);
#else
	::InitializeCriticalSection(&m_lock);
#endif
}

void CLock::Destroy()
{
	assert(!m_bLocked && "lock didn't released");
#ifndef OS_WINDOWS
	pthread_mutex_destroy(&m_lock);
#else
	::DeleteCriticalSection(&m_lock);
#endif
}

int CLock::Trylock()
{
#ifndef OS_WINDOWS
	return pthread_mutex_trylock(&m_lock);
#else
	return ::TryEnterCriticalSection(&m_lock);
#endif
}

void CLock::Lock()
{
#ifndef OS_WINDOWS
	pthread_mutex_lock(&m_lock);
#else
	::EnterCriticalSection(&m_lock);
#endif
	m_bLocked = true;
}

void CLock::Unlock()
{
	assert(m_bLocked && "lock didn't issued");
	m_bLocked = false;
#ifndef OS_WINDOWS
	pthread_mutex_unlock(&m_lock);
#else
	::LeaveCriticalSection(&m_lock);
#endif
}
