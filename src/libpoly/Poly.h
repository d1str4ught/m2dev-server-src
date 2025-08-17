#ifndef __POLY_POLY_H__
#define __POLY_POLY_H__

#include "SymTable.h"

#include <string>
#include <vector>
#include <list>

class CPoly
{
	public:
		CPoly();
		virtual ~CPoly();

		int	Analyze(const char * pszStr = NULL);
		double	Eval();
		void	SetStr(const std::string & str);
		int	SetVar(const std::string & strName, double dVar);
		double GetVar(const std::string & strName);
		void	Clear();

	protected:
		int		my_irandom(double start, double end);
		double		my_frandom(double start, double end);

		void		init();
		int		insert(const std::string & s, int tok);
		int		find(const std::string & s);
		void		emit(int t,int tval);
		void		match(int t); 
		void		expo(); 
		void		factor(); 
		void		term(); 
		int		lexan();
		void		error();
		void		expr();

		int		iToken;
		double		iNumToken;
		int		iLookAhead;
		int		iErrorPos;
		bool		ErrorOccur;
		unsigned int	uiLookPos;

		// NOTE: list is slight faster than vector, why?!
		std::vector<int>		tokenBase;
		std::vector<double>		numBase;
		std::vector<CSymTable *>	lSymbol;
		std::vector<int>		SymbolIndex;
		int				STSize;
		std::string			strData;

};

#endif 
