#include "Symbol.h"

CSymbol::CSymbol()
{
    id = MID_SYMBOL;
    iType = ST_UNKNOWN;
}

CSymbol::~CSymbol()
{

}

bool CSymbol::Equal(CSymbol dif)
{
    if (dif.iType/10 == iType/10) 
	return true;
    return false;
}

bool CSymbol::Less(CSymbol dif)
{
    if (dif.iType/10 > iType/10) return true;
    return false;
}

int CSymbol::GetType()
{
    return iType;
}

void CSymbol::SetType(int Type)
{
    iType=Type;
}

int CSymbol::issymbol(int ch)
{
    switch(ch)
    {
	case SY_PLUS	: return ST_PLUS;
	case SY_MINUS	: return ST_MINUS;
	case SY_MULTIPLY: return ST_MULTIPLY;
	case SY_DIVIDE	: return SY_DIVIDE;
	case SY_CARET	: return SY_CARET;
	case SY_OPEN	: return SY_OPEN;
	case SY_CLOSE	: return ST_CLOSE;
    }
    return 0;
}

