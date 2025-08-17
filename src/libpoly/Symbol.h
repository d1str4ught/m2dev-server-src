#ifndef __POLY_SYMBOL_H__
#define __POLY_SYMBOL_H__

#include "Base.h"

#define ST_UNKNOWN	0
#define ST_PLUS		11
#define ST_MINUS	12
#define ST_MULTIPLY	23
#define ST_DIVIDE	24
#define ST_CARET	35
#define ST_OPEN		06
#define ST_CLOSE	07

#define SY_PLUS		'+'
#define SY_MINUS	'-'
#define SY_MULTIPLY	'*'
#define SY_DIVIDE	'/'
#define SY_CARET	'^'
#define SY_OPEN		'('
#define SY_CLOSE	')'

class CSymbol : public CBase   
{
    private:
	int	iType;

    public:
	CSymbol();
	virtual ~CSymbol();

	static int	issymbol(int ch);
	void		SetType(int Type);
	int		GetType();
	bool		Equal(CSymbol dif);
	bool		Less(CSymbol dif);
};

#endif 
