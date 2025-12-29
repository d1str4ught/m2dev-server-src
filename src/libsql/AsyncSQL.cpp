#ifndef OS_WINDOWS
#include <sys/time.h>
#endif

#include <cstdlib>
#include <cstring>
#include <chrono>

#include "AsyncSQL.h"

CAsyncSQL::CAsyncSQL()
	: m_stHost(""), m_stUser(""), m_stPassword(""), m_stDB(""), m_stLocale(""),
	m_iPort(0), m_thread(nullptr), m_bEnd(false), m_bConnected(false),
	m_iMsgCount(0), m_iQueryFinished(0), m_iCopiedQuery(0), m_ulThreadID(0)
{
	memset(&m_hDB, 0, sizeof(m_hDB));
}

CAsyncSQL::~CAsyncSQL()
{
	Quit();
	Destroy();
}

void CAsyncSQL::Destroy()
{
	if (m_hDB.host)
	{
		sys_log(0, "AsyncSQL: closing mysql connection.");
		mysql_close(&m_hDB);
		m_hDB.host = nullptr;
	}
}

bool CAsyncSQL::QueryLocaleSet()
{
	if (m_stLocale.empty())
	{
		sys_err("m_stLocale == 0");
		return true;
	}

	if (m_stLocale == "ascii")
	{
		sys_err("m_stLocale == ascii");
		return true;
	}

	if (mysql_set_character_set(&m_hDB, m_stLocale.c_str()))
	{
		sys_err("cannot set locale %s by 'mysql_set_character_set', errno %u %s",
			m_stLocale.c_str(), mysql_errno(&m_hDB), mysql_error(&m_hDB));
		return false;
	}

	sys_log(0, "\t--mysql_set_character_set(%s)", m_stLocale.c_str());
	return true;
}

bool CAsyncSQL::Connect()
{
	if (mysql_init(&m_hDB) == nullptr)
	{
		fprintf(stderr, "mysql_init failed\n");
		return false;
	}

	if (!m_stLocale.empty())
	{
		if (mysql_options(&m_hDB, MYSQL_SET_CHARSET_NAME, m_stLocale.c_str()) != 0)
		{
			fprintf(stderr, "mysql_option failed : MYSQL_SET_CHARSET_NAME %s ", mysql_error(&m_hDB));
		}
	}

	if (!mysql_real_connect(&m_hDB, m_stHost.c_str(), m_stUser.c_str(),
		m_stPassword.c_str(), m_stDB.c_str(), m_iPort, nullptr, CLIENT_MULTI_STATEMENTS))
	{
		fprintf(stderr, "mysql_real_connect: %s\n", mysql_error(&m_hDB));
		return false;
	}

	my_bool reconnect = true;
	if (mysql_options(&m_hDB, MYSQL_OPT_RECONNECT, &reconnect) != 0)
	{
		fprintf(stderr, "mysql_option: %s\n", mysql_error(&m_hDB));
	}

	fprintf(stdout, "AsyncSQL: connected to %s (reconnect %d)\n", m_stHost.c_str(), reconnect);

	m_ulThreadID.store(mysql_thread_id(&m_hDB), std::memory_order_release);
	m_bConnected.store(true, std::memory_order_release);
	return true;
}

bool CAsyncSQL::Setup(CAsyncSQL* sql, bool bNoThread)
{
	return Setup(sql->m_stHost.c_str(),
		sql->m_stUser.c_str(),
		sql->m_stPassword.c_str(),
		sql->m_stDB.c_str(),
		sql->m_stLocale.c_str(),
		bNoThread,
		sql->m_iPort);
}

bool CAsyncSQL::Setup(const char* c_pszHost, const char* c_pszUser, const char* c_pszPassword,
	const char* c_pszDB, const char* c_pszLocale, bool bNoThread, int iPort)
{
	m_stHost = c_pszHost;
	m_stUser = c_pszUser;
	m_stPassword = c_pszPassword;
	m_stDB = c_pszDB;
	m_iPort = iPort;

	if (c_pszLocale)
	{
		m_stLocale = c_pszLocale;
		sys_log(0, "AsyncSQL: locale %s", m_stLocale.c_str());
	}

	if (!bNoThread)
	{
		// Create worker thread using modern C++ thread
		m_thread = std::make_unique<std::thread>([this]() {
			if (!Connect())
				return;
			ChildLoop();
		});

		return true;
	}
	else
	{
		return Connect();
	}
}

void CAsyncSQL::Quit()
{
	m_bEnd.store(true, std::memory_order_release);
	m_cvQuery.notify_all();

	if (m_thread && m_thread->joinable())
	{
		m_thread->join();
		m_thread.reset();
	}
}

std::unique_ptr<SQLMsg> CAsyncSQL::DirectQuery(const char* c_pszQuery)
{
	unsigned long currentThreadID = mysql_thread_id(&m_hDB);
	if (m_ulThreadID.load(std::memory_order_acquire) != currentThreadID)
	{
		sys_err("MySQL connection was reconnected. querying locale set");
		while (!QueryLocaleSet());
		m_ulThreadID.store(currentThreadID, std::memory_order_release);
	}

	auto p = std::make_unique<SQLMsg>();
	p->m_pkSQL = &m_hDB;
	p->iID = m_iMsgCount.fetch_add(1, std::memory_order_acq_rel) + 1;
	p->stQuery = c_pszQuery;

	if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
	{
		char buf[1024];
		snprintf(buf, sizeof(buf),
			"AsyncSQL::DirectQuery : mysql_query error: %s\nquery: %s",
			mysql_error(&m_hDB), p->stQuery.c_str());

		sys_err(buf);
		p->uiSQLErrno = mysql_errno(&m_hDB);
	}

	p->Store();
	return p;
}

void CAsyncSQL::AsyncQuery(const char* c_pszQuery)
{
	auto p = std::make_unique<SQLMsg>();
	p->m_pkSQL = &m_hDB;
	p->iID = m_iMsgCount.fetch_add(1, std::memory_order_acq_rel) + 1;
	p->stQuery = c_pszQuery;

	PushQuery(std::move(p));
}

void CAsyncSQL::ReturnQuery(const char* c_pszQuery, void* pvUserData)
{
	auto p = std::make_unique<SQLMsg>();
	p->m_pkSQL = &m_hDB;
	p->iID = m_iMsgCount.fetch_add(1, std::memory_order_acq_rel) + 1;
	p->stQuery = c_pszQuery;
	p->bReturn = true;
	p->pvUserData = pvUserData;

	PushQuery(std::move(p));
}

void CAsyncSQL::PushResult(std::unique_ptr<SQLMsg> p)
{
	std::lock_guard<std::mutex> lock(m_mtxResult);
	m_queue_result.push(std::move(p));
}

bool CAsyncSQL::PopResult(std::unique_ptr<SQLMsg>& p)
{
	std::lock_guard<std::mutex> lock(m_mtxResult);

	if (m_queue_result.empty())
		return false;

	p = std::move(m_queue_result.front());
	m_queue_result.pop();
	return true;
}

// Legacy API for backward compatibility
bool CAsyncSQL::PopResult(SQLMsg** pp)
{
	std::lock_guard<std::mutex> lock(m_mtxResult);

	if (m_queue_result.empty())
		return false;

	*pp = m_queue_result.front().release();
	m_queue_result.pop();
	return true;
}

void CAsyncSQL::PushQuery(std::unique_ptr<SQLMsg> p)
{
	{
		std::lock_guard<std::mutex> lock(m_mtxQuery);
		m_queue_query.push(std::move(p));
	}
	m_cvQuery.notify_one();
}

bool CAsyncSQL::PeekQuery(SQLMsg** pp)
{
	std::lock_guard<std::mutex> lock(m_mtxQuery);

	if (m_queue_query.empty())
		return false;

	*pp = m_queue_query.front().get();
	return true;
}

bool CAsyncSQL::PopQuery(int iID)
{
	std::lock_guard<std::mutex> lock(m_mtxQuery);

	if (m_queue_query.empty())
		return false;

	m_queue_query.pop();
	return true;
}

bool CAsyncSQL::PeekQueryFromCopyQueue(SQLMsg** pp)
{
	if (m_queue_query_copy.empty())
		return false;

	*pp = m_queue_query_copy.front().get();
	return true;
}

int CAsyncSQL::CopyQuery()
{
	std::lock_guard<std::mutex> lock(m_mtxQuery);

	if (m_queue_query.empty())
		return -1;

	while (!m_queue_query.empty())
	{
		m_queue_query_copy.push(std::move(m_queue_query.front()));
		m_queue_query.pop();
	}

	return static_cast<int>(m_queue_query_copy.size());
}

bool CAsyncSQL::PopQueryFromCopyQueue()
{
	if (m_queue_query_copy.empty())
		return false;

	m_queue_query_copy.pop();
	return true;
}

int CAsyncSQL::GetCopiedQueryCount() const
{
	return m_iCopiedQuery.load(std::memory_order_acquire);
}

void CAsyncSQL::ResetCopiedQueryCount()
{
	m_iCopiedQuery.store(0, std::memory_order_release);
}

void CAsyncSQL::AddCopiedQueryCount(int iCopiedQuery)
{
	m_iCopiedQuery.fetch_add(iCopiedQuery, std::memory_order_acq_rel);
}

DWORD CAsyncSQL::CountQuery()
{
	std::lock_guard<std::mutex> lock(m_mtxQuery);
	return static_cast<DWORD>(m_queue_query.size());
}

DWORD CAsyncSQL::CountResult()
{
	std::lock_guard<std::mutex> lock(m_mtxResult);
	return static_cast<DWORD>(m_queue_result.size());
}

// Modern profiler using chrono
class cProfiler
{
	public:
		cProfiler(int nInterval = 500000)
			: m_nInterval(nInterval)
		{
			Start();
		}

		void Start()
		{
			m_start = std::chrono::steady_clock::now();
		}

		void Stop()
		{
			m_end = std::chrono::steady_clock::now();
		}

		bool IsOk() const
		{
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start);
			return duration.count() <= m_nInterval;
		}

		long GetResultSec() const
		{
			auto duration = std::chrono::duration_cast<std::chrono::seconds>(m_end - m_start);
			return static_cast<long>(duration.count());
		}

		long GetResultUSec() const
		{
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start);
			return static_cast<long>(duration.count() % 1000000);
		}

	private:
		int m_nInterval;
		std::chrono::steady_clock::time_point m_start;
		std::chrono::steady_clock::time_point m_end;
};

void CAsyncSQL::ChildLoop()
{
	cProfiler profiler(500000); // 0.5 seconds

	while (!m_bEnd.load(std::memory_order_acquire))
	{
		// Wait for queries using condition variable
		std::unique_lock<std::mutex> lock(m_mtxQuery);
		m_cvQuery.wait(lock, [this] {
			return !m_queue_query.empty() || m_bEnd.load(std::memory_order_acquire);
		});
		lock.unlock();

		if (m_bEnd.load(std::memory_order_acquire) && m_queue_query.empty())
			break;

		int count = CopyQuery();
		if (count <= 0)
			continue;

		AddCopiedQueryCount(count);

		while (count--)
		{
			if (m_queue_query_copy.empty())
				continue;

			// Peek first, don't pop yet (for retry logic)
			SQLMsg* p = m_queue_query_copy.front().get();
			bool shouldRetry = false;

			profiler.Start();

			// Check for reconnection
			unsigned long currentThreadID = mysql_thread_id(&m_hDB);
			if (m_ulThreadID.load(std::memory_order_acquire) != currentThreadID)
			{
				sys_err("MySQL connection was reconnected. querying locale set");
				while (!QueryLocaleSet());
				m_ulThreadID.store(currentThreadID, std::memory_order_release);
			}

			if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
			{
				p->uiSQLErrno = mysql_errno(&m_hDB);

				sys_err("AsyncSQL: query failed: %s (query: %s errno: %d)",
					mysql_error(&m_hDB), p->stQuery.c_str(), p->uiSQLErrno);

				// Retry on connection errors
				switch (p->uiSQLErrno)
				{
				case CR_SOCKET_CREATE_ERROR:
				case CR_CONNECTION_ERROR:
				case CR_IPSOCK_ERROR:
				case CR_UNKNOWN_HOST:
				case CR_SERVER_GONE_ERROR:
				case CR_CONN_HOST_ERROR:
				case ER_NOT_KEYFILE:
				case ER_CRASHED_ON_USAGE:
				case ER_CANT_OPEN_FILE:
				case ER_HOST_NOT_PRIVILEGED:
				case ER_HOST_IS_BLOCKED:
				case ER_PASSWORD_NOT_ALLOWED:
				case ER_PASSWORD_NO_MATCH:
				case ER_CANT_CREATE_THREAD:
				case ER_INVALID_USE_OF_NULL:
					sys_err("AsyncSQL: retrying");
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					shouldRetry = true;
					break;
				}
			}

			profiler.Stop();

			// If retry, don't pop - continue to next iteration
			if (shouldRetry)
				continue;

			// Log slow queries (> 0.5 seconds)
			if (!profiler.IsOk())
			{
				sys_log(0, "[QUERY : LONG INTERVAL(OverSec %ld.%ld)] : %s",
					profiler.GetResultSec(), profiler.GetResultUSec(), p->stQuery.c_str());
			}

			// Now pop and move ownership
			auto pMsg = std::move(m_queue_query_copy.front());
			m_queue_query_copy.pop();

			if (p->bReturn)
			{
				p->Store();
				// Move ownership to result queue
				PushResult(std::move(pMsg));
			}
			// else: pMsg will be automatically deleted when it goes out of scope

			m_iQueryFinished.fetch_add(1, std::memory_order_acq_rel);
		}
	}

	// Process remaining queries during shutdown
	{
		std::lock_guard<std::mutex> lock(m_mtxQuery);

		while (!m_queue_query.empty())
		{
			auto pMsg = std::move(m_queue_query.front());
			SQLMsg* p = pMsg.get();
			m_queue_query.pop();

			unsigned long currentThreadID = mysql_thread_id(&m_hDB);
			if (m_ulThreadID.load(std::memory_order_acquire) != currentThreadID)
			{
				sys_err("MySQL connection was reconnected. querying locale set");
				while (!QueryLocaleSet());
				m_ulThreadID.store(currentThreadID, std::memory_order_release);
			}

			if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
			{
				p->uiSQLErrno = mysql_errno(&m_hDB);

				sys_err("AsyncSQL::ChildLoop : mysql_query error: %s:\nquery: %s",
					mysql_error(&m_hDB), p->stQuery.c_str());

				// Retry on connection errors
				switch (p->uiSQLErrno)
				{
				case CR_SOCKET_CREATE_ERROR:
				case CR_CONNECTION_ERROR:
				case CR_IPSOCK_ERROR:
				case CR_UNKNOWN_HOST:
				case CR_SERVER_GONE_ERROR:
				case CR_CONN_HOST_ERROR:
				case ER_NOT_KEYFILE:
				case ER_CRASHED_ON_USAGE:
				case ER_CANT_OPEN_FILE:
				case ER_HOST_NOT_PRIVILEGED:
				case ER_HOST_IS_BLOCKED:
				case ER_PASSWORD_NOT_ALLOWED:
				case ER_PASSWORD_NO_MATCH:
				case ER_CANT_CREATE_THREAD:
				case ER_INVALID_USE_OF_NULL:
					continue;
				}
			}

			sys_log(0, "QUERY_FLUSH: %s", p->stQuery.c_str());

			if (p->bReturn)
			{
				p->Store();
				PushResult(std::move(pMsg));
			}

			m_iQueryFinished.fetch_add(1, std::memory_order_acq_rel);
		}
	}
}

int CAsyncSQL::CountQueryFinished() const
{
	return m_iQueryFinished.load(std::memory_order_acquire);
}

void CAsyncSQL::ResetQueryFinished()
{
	m_iQueryFinished.store(0, std::memory_order_release);
}

MYSQL* CAsyncSQL::GetSQLHandle()
{
	return &m_hDB;
}

size_t CAsyncSQL::EscapeString(char* dst, size_t dstSize, const char* src, size_t srcSize)
{
	if (srcSize == 0)
	{
		memset(dst, 0, dstSize);
		return 0;
	}

	if (dstSize == 0)
		return 0;

	if (dstSize < srcSize * 2 + 1)
	{
		char tmp[256];
		size_t tmpLen = sizeof(tmp) > srcSize ? srcSize : sizeof(tmp);
		strlcpy(tmp, src, tmpLen);

		sys_err("FATAL ERROR!! not enough buffer size (dstSize %u srcSize %u src%s: %s)",
			static_cast<unsigned int>(dstSize), static_cast<unsigned int>(srcSize),
			tmpLen != srcSize ? "(trimmed to 255 characters)" : "", tmp);

		dst[0] = '\0';
		return 0;
	}

	return mysql_real_escape_string(GetSQLHandle(), dst, src, srcSize);
}

void CAsyncSQL2::SetLocale(const std::string& stLocale)
{
	m_stLocale = stLocale;
	QueryLocaleSet();
}
