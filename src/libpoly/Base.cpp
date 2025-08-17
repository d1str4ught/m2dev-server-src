#include "Base.h"

CBase::CBase()
{
    id = 0;
}

CBase::~CBase()
{
}

bool CBase::isNumber()
{
    return (id & MID_NUMBER);
}

bool CBase::isVar()
{
    return (id & MID_VARIABLE);
}

bool CBase::isSymbol()
{
    return (id & MID_SYMBOL);
}

