#ifndef __INC_METIN_II_ASYNCSQL_H__
#define __INC_METIN_II_ASYNCSQL_H__

#include "libthecore/stdafx.h"
#include "libthecore/log.h"

#include <string>
#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#include <mysql.h>
#include <errmsg.h>
#include <mysqld_error.h>

#define QUERY_MAX_LEN 8192

// Modern RAII wrapper for MySQL results
struct SQLResult
{
	SQLResult() noexcept
		: pSQLResult(nullptr), uiNumRows(0), uiAffectedRows(0), uiInsertID(0)
	{
	}

	~SQLResult()
	{
		if (pSQLResult)
		{
			mysql_free_result(pSQLResult);
			pSQLResult = nullptr;
		}
	}

	// Delete copy constructor and assignment operator (non-copyable)
	SQLResult(const SQLResult&) = delete;
	SQLResult& operator=(const SQLResult&) = delete;

	// Allow move semantics
	SQLResult(SQLResult&& other) noexcept
		: pSQLResult(other.pSQLResult),
		uiNumRows(other.uiNumRows),
		uiAffectedRows(other.uiAffectedRows),
		uiInsertID(other.uiInsertID)
	{
		other.pSQLResult = nullptr;
		other.uiNumRows = 0;
		other.uiAffectedRows = 0;
		other.uiInsertID = 0;
	}

	SQLResult& operator=(SQLResult&& other) noexcept
	{
		if (this != &other)
		{
			if (pSQLResult)
			{
				mysql_free_result(pSQLResult);
			}

			pSQLResult = other.pSQLResult;
			uiNumRows = other.uiNumRows;
			uiAffectedRows = other.uiAffectedRows;
			uiInsertID = other.uiInsertID;

			other.pSQLResult = nullptr;
			other.uiNumRows = 0;
			other.uiAffectedRows = 0;
			other.uiInsertID = 0;
		}
		return *this;
	}

	MYSQL_RES*	pSQLResult;
	uint32_t	uiNumRows;
	uint32_t	uiAffectedRows;
	uint32_t	uiInsertID;
};

// SQL Message with improved memory management
typedef struct _SQLMsg
{
	_SQLMsg() noexcept
		: m_pkSQL(nullptr), iID(0), uiResultPos(0), pvUserData(nullptr),
		bReturn(false), uiSQLErrno(0)
	{
	}

	~_SQLMsg()
	{
		vec_pkResult.clear();
	}

	// Delete copy operations
	_SQLMsg(const _SQLMsg&) = delete;
	_SQLMsg& operator=(const _SQLMsg&) = delete;

	// Allow move operations
	_SQLMsg(_SQLMsg&&) noexcept = default;
	_SQLMsg& operator=(_SQLMsg&&) noexcept = default;

	void Store()
	{
		do
		{
			auto pRes = std::make_unique<SQLResult>();

			pRes->pSQLResult = mysql_store_result(m_pkSQL);
			pRes->uiInsertID = static_cast<uint32_t>(mysql_insert_id(m_pkSQL));
			pRes->uiAffectedRows = static_cast<uint32_t>(mysql_affected_rows(m_pkSQL));

			if (pRes->pSQLResult)
			{
				pRes->uiNumRows = static_cast<uint32_t>(mysql_num_rows(pRes->pSQLResult));
			}
			else
			{
				pRes->uiNumRows = 0;
			}

			vec_pkResult.push_back(std::move(pRes));
		} while (mysql_next_result(m_pkSQL) == 0);
	}

	SQLResult* Get()
	{
		if (uiResultPos >= vec_pkResult.size())
			return nullptr;

		return vec_pkResult[uiResultPos].get();
	}

	bool Next()
	{
		if (uiResultPos + 1 >= vec_pkResult.size())
			return false;

		++uiResultPos;
		return true;
	}

	MYSQL*								m_pkSQL;
	int									iID;
	std::string							stQuery;
	std::vector<std::unique_ptr<SQLResult>>	vec_pkResult;
	unsigned int						uiResultPos;
	void*								pvUserData;
	bool								bReturn;
	unsigned int						uiSQLErrno;
} SQLMsg;

class CAsyncSQL
{
	public:
		CAsyncSQL();
		virtual ~CAsyncSQL();

		void Quit();

		bool Setup(const char* c_pszHost, const char* c_pszUser, const char* c_pszPassword,
			const char* c_pszDB, const char* c_pszLocale, bool bNoThread = false, int iPort = 0);
		bool Setup(CAsyncSQL* sql, bool bNoThread = false);

		bool Connect();
		bool IsConnected() const { return m_bConnected.load(std::memory_order_acquire); }
		bool QueryLocaleSet();

		void AsyncQuery(const char* c_pszQuery);
		void ReturnQuery(const char* c_pszQuery, void* pvUserData);
		std::unique_ptr<SQLMsg> DirectQuery(const char* c_pszQuery);

		DWORD CountQuery();
		DWORD CountResult();

		void PushResult(std::unique_ptr<SQLMsg> p);
		bool PopResult(std::unique_ptr<SQLMsg>& p);

		// Legacy API compatibility - deprecated, use PopResult(unique_ptr&) instead
		bool PopResult(SQLMsg** pp);

		void ChildLoop();

		MYSQL* GetSQLHandle();

		int CountQueryFinished() const;
		void ResetQueryFinished();

		size_t EscapeString(char* dst, size_t dstSize, const char* src, size_t srcSize);

	protected:
		void Destroy();
		void PushQuery(std::unique_ptr<SQLMsg> p);
		bool PeekQuery(SQLMsg** pp);
		bool PopQuery(int iID);
		bool PeekQueryFromCopyQueue(SQLMsg** pp);
		int CopyQuery();
		bool PopQueryFromCopyQueue();

	public:
		int GetCopiedQueryCount() const;
		void ResetCopiedQueryCount();
		void AddCopiedQueryCount(int iCopiedQuery);

	protected:
		// MySQL connection
		MYSQL m_hDB;

		// Connection info
		std::string m_stHost;
		std::string m_stUser;
		std::string m_stPassword;
		std::string m_stDB;
		std::string m_stLocale;
		int m_iPort;

		// Thread control
		std::unique_ptr<std::thread> m_thread;
		std::atomic<bool> m_bEnd;
		std::atomic<bool> m_bConnected;

		// Query queues with mutex protection
		std::queue<std::unique_ptr<SQLMsg>> m_queue_query;
		std::queue<std::unique_ptr<SQLMsg>> m_queue_query_copy;
		std::queue<std::unique_ptr<SQLMsg>> m_queue_result;

		std::mutex m_mtxQuery;
		std::mutex m_mtxResult;
		std::condition_variable m_cvQuery;

		// Counters
		std::atomic<int> m_iMsgCount;
		std::atomic<int> m_iQueryFinished;
		std::atomic<int> m_iCopiedQuery;
		std::atomic<unsigned long> m_ulThreadID;
};

class CAsyncSQL2 : public CAsyncSQL
{
	public:
		void SetLocale(const std::string& stLocale);
};

#endif
