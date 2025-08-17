#include "Statement.h"

#include <cstdlib>
#include <cstring>

CStmt::CStmt()
{
	m_pkStmt = NULL;
	m_uiParamCount = 0;
	m_uiResultCount = 0;
	iRows = 0;
	m_puiParamLen = NULL;
}

CStmt::~CStmt()
{
	Destroy();
}

void CStmt::Destroy()
{
	if (m_pkStmt)
	{
		mysql_stmt_close(m_pkStmt);
		m_pkStmt = NULL;
	}

	if (m_puiParamLen)
	{
		free(m_puiParamLen);
		m_puiParamLen = NULL;
	}
}

void CStmt::Error(const char * c_pszMsg)
{
	sys_log(0, "SYSERR: %s: [%d] %s", c_pszMsg, mysql_stmt_errno(m_pkStmt), mysql_stmt_error(m_pkStmt));
}

bool CStmt::Prepare(CAsyncSQL * sql, const char * c_pszQuery)
{
	m_pkStmt = mysql_stmt_init(sql->GetSQLHandle());
	m_stQuery = c_pszQuery;

	if (mysql_stmt_prepare(m_pkStmt, m_stQuery.c_str(), m_stQuery.length()))
	{
		Error("mysql_stmt_prepare");
		return false;
	}

	int iParamCount = 0;

	for (unsigned int i = 0; i < m_stQuery.length(); ++i)
		if (c_pszQuery[i] == '?')
			++iParamCount;

	if (iParamCount)
	{
		m_vec_param.resize(iParamCount);
		memset(&m_vec_param[0], 0, sizeof(MYSQL_BIND) * iParamCount);

		m_puiParamLen = (long unsigned int *) calloc(iParamCount, sizeof(long unsigned int));
	}

	m_vec_result.resize(48);
	memset(&m_vec_result[0], 0, sizeof(MYSQL_BIND) * 48);

	if (mysql_stmt_bind_result(m_pkStmt, &m_vec_result[0]))
	{
		Error("mysql_stmt_bind_result");
		return 0;
	}

	return true;
}

bool CStmt::BindParam(enum_field_types type, void * p, int iMaxLen)
{
	if (m_uiParamCount >= m_vec_param.size())
	{
		sys_err("too many parameter in query: %s", m_stQuery.c_str());
		return false;
	}

	MYSQL_BIND * bind = &m_vec_param[m_uiParamCount];

	bind->buffer_type	= type;
	bind->buffer	= (void *) p;
	bind->buffer_length	= iMaxLen;
	bind->length	= m_puiParamLen + m_uiParamCount;

	if (++m_uiParamCount == m_vec_param.size())
	{
		if (mysql_stmt_bind_param(m_pkStmt, &m_vec_param[0]))
		{
			Error("mysql_stmt_bind_param");
			return false;
		}
	}

	return true;
}

bool CStmt::BindResult(enum_field_types type, void * p, int iMaxLen)
{
	if (m_uiResultCount >= m_vec_result.size())
	{
		sys_err("too many result in query: %s", m_stQuery.c_str());
		return false;
	}

	MYSQL_BIND * bind = &m_vec_result[m_uiResultCount++];

	bind->buffer_type	= type;
	bind->buffer	= (void *) p;
	bind->buffer_length	= iMaxLen;
	return true;
}

int CStmt::Execute()
{
	if (m_uiParamCount != m_vec_param.size())
	{
		sys_log(0, "Parameter not enough %d, expected %d query: %s", m_uiParamCount, m_vec_param.size(), m_stQuery.c_str());
		return 0;
	}

	for (unsigned int i = 0; i < m_uiParamCount; ++i)
	{
		MYSQL_BIND * bind = &m_vec_param[i];

		if (bind->buffer_type == MYSQL_TYPE_STRING)
		{
			*(m_puiParamLen + i) = strlen((const char *) bind->buffer);
			sys_log(0, "param %d len %d buf %s", i, *m_puiParamLen, (const char *) bind->buffer);
		}
	}

	if (mysql_stmt_execute(m_pkStmt))
	{
		Error("mysql_stmt_execute");
		return 0;
	}

	if (mysql_stmt_store_result(m_pkStmt))
	{
		Error("mysql_store_result");
		return 0;
	}

	iRows = mysql_stmt_num_rows(m_pkStmt);
	return true;
}

bool CStmt::Fetch()
{
	return !mysql_stmt_fetch(m_pkStmt);
}

