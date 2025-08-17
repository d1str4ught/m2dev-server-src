#ifndef __POLY_BASE_H__
#define __POLY_BASE_H__

#define MID_UNKNOWN		0
#define MID_NUMBER		256
#define MID_VARIABLE		512
#define MID_SYMBOL		1024

#define MID_LONG		MID_NUMBER + 1
#define MID_SQRT		MID_NUMBER + 2
#define MID_FRACTION		MID_NUMBER + 3

class CBase  
{
    public:
	bool isSymbol();
	bool isVar();
	bool isNumber();
	int id;
	CBase();
	virtual ~CBase();

};

#endif
