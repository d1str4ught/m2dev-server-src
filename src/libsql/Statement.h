#ifndef __INC_METIN_II_LIBSQL_STATEMENT_H__
#define __INC_METIN_II_LIBSQL_STATEMENT_H__

#include "AsyncSQL.h"

#include <string>
#include <vector>

class CStmt
{
	public:
		CStmt();
		virtual ~CStmt();

		bool    Prepare(CAsyncSQL * sql, const char * c_pszQuery);
		bool    BindParam(enum_field_types type, void * p, int iMaxLen=0);
		bool    BindResult(enum_field_types type, void * p, int iMaxLen=0);
		int	Execute();
		bool    Fetch();

		void    Error(const char * c_pszMsg);

	public:
		int	iRows;

	private:
		void    Destroy();

		MYSQL_STMT *            m_pkStmt;

		std::string             m_stQuery;

		std::vector<MYSQL_BIND> m_vec_param;
		unsigned int            m_uiParamCount;
		long unsigned int *	m_puiParamLen;

		std::vector<MYSQL_BIND> m_vec_result;
		unsigned int            m_uiResultCount;
};  

#endif
