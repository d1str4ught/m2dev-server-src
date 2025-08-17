#include <string>
#include "Poly.h"
#include "Constants.h"
#include <cmath>
#include <cctype>
#include <cstdlib>

using namespace std;

double _random()
{
#ifndef __WIN32__
    return random() / (2147483648.0);
#else
	return rand() / (2147483648.0);
#endif
}

int CPoly::my_irandom(double start, double end)
{
    // Make range as inclusive-exclusive
    int is = int(start + 0.5);
    int ie = int(end - start + 0.5) + 1;

    return int(_random() * ie + is);
}

double CPoly::my_frandom(double start, double end)
{
    return _random() * (end - start) + start;
}

CPoly::CPoly()
	: iToken(0), iNumToken(0), iLookAhead(0), iErrorPos(0), ErrorOccur(true),
	uiLookPos(0), STSize(0)
{
    lSymbol.clear();
    lSymbol.reserve(50);

    init();
}

CPoly::~CPoly()
{
    Clear();
}

void CPoly::SetStr(const string & str)
{
    strData = str;
}

double CPoly::Eval()
{
	int stNow;
	double save[MAXSTACK],t;
	int iSp=0;
	if (ErrorOccur) 
	{
		/*THROW(new CEvalException("Evaluate Error"));*/ 
		return 0;
	}

	//TEST
	//list<int>::iterator pos = tokenBase.begin();
	//list<double>::iterator posn = numBase.begin();
	vector<int>::iterator pos = tokenBase.begin();
	vector<double>::iterator posn = numBase.begin();
	while (pos != tokenBase.end())
	{

		stNow=*pos;
		++pos;
		switch (stNow)
		{
			case NUM:
				save[iSp++]=*posn++; break;
			case ID:
				save[iSp++]=
					lSymbol[ *pos ]->dVal; 
				pos++;
				break;
				//case '+':
			case PLU:
				iSp--;
				save[iSp-1]+=save[iSp]; break;
				//case '-':
			case MIN:
				iSp--;
				save[iSp-1]-=save[iSp]; break;
				//case '*':
			case MUL:
				iSp--;
				save[iSp-1]*=save[iSp]; break;
				//case '%':
			case MOD:
				iSp--;
				if (save[iSp]==0) 
				{
					//THROW(new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]=fmod(save[iSp-1],save[iSp]); break;
				//case '/':
			case DIV:
				iSp--;
				if (save[iSp]==0) 
				{
					//THROW(new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]/=save[iSp]; break;
				//case '^':
			case POW:
				iSp--;
				save[iSp-1]=pow(save[iSp-1],save[iSp]); break;
			case ROOT:
				if (save[iSp-1]<0) 
				{
					//THROW(new CEvalException("Negative in root"));
					return 0;
				}
				save[iSp-1]=sqrt(save[iSp-1]); break;
			case COS:
				save[iSp-1]=cos(save[iSp-1]); break;
			case SIN:
				save[iSp-1]=sin(save[iSp-1]); break;
			case SIGN:
				if (save[iSp-1] == 0.0) save[iSp-1] = 0.0;
				else if (save[iSp-1] < 0.0) save[iSp-1] = -1.0;
				else save[iSp-1] = 1.0f;
				break;
			case TAN:
				if (!(t=cos(save[iSp-1]))) 
				{
					//THROW (new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]=tan(save[iSp-1]); break;
			case CSC:
				if (!(t=sin(save[iSp-1]))) 
				{
					//THROW(new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]=1/t; break;
			case SEC:
				if (!(t=cos(save[iSp-1]))) 
				{
					//THROW(new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]=1/t; break;
			case COT:
				if (!(t=sin(save[iSp-1]))) 
				{
					//THROW(new CEvalException("Divide by 0"));
					return 0;
				}
				save[iSp-1]=cos(save[iSp-1])/t; break;
			case LN:
				if (save[iSp-1]<=0) 
				{
					//THROW( new CEvalException("Call Log with minus number"));
					return 0;
				}
				save[iSp-1]=log(save[iSp-1]); break;
			case LOG10:
				if (save[iSp-1]<=0) 
				{
					//THROW( new CEvalException("Call Log with minus number"));
					return 0;
				}
				save[iSp-1]=log10(save[iSp-1]); break;
			case LOG:
				if (save[iSp-1]<=0) 
				{
					//THROW( new CEvalException("Call Log with minus number"));
					return 0;
				}
				if (save[iSp-2]<=0 || save[iSp-2]==1) 
				{
					//THROW( new CEvalException("Call Log with minus number"));
					return 0;
				}

				save[iSp-2]=log(save[iSp-1])/log(save[iSp-2]);
				iSp--;
				break;
			case ABS:	save[iSp-1]=fabs(save[iSp-1]); break;
			case FLOOR:	save[iSp-1]=floor(save[iSp-1]); break;
			case IRAND:
						save[iSp-2]=my_irandom(save[iSp-2],save[iSp-1]);
						iSp--;
						break;
			case FRAND:
						save[iSp-2]=my_frandom(save[iSp-2],save[iSp-1]);
						iSp--;
						break;
			case MINF:
						save[iSp-2]=(save[iSp-2]<save[iSp-1])?save[iSp-2]:save[iSp-1];
						iSp--;
						break;
			case MAXF:
						save[iSp-2]=(save[iSp-2]>save[iSp-1])?save[iSp-2]:save[iSp-1];
						iSp--;
						break;
						/*case MOD:
						  save[iSp-2]=fmod(save[iSp-2],save[iSp-1]);
						  iSp--;
						  break;*/
			default:
						return 0;
						//THROW(new CEvalException("Token Error"));
		}
	}
	return save[iSp-1];
}

int CPoly::Analyze(const char * pszStr)
{
    if (pszStr)
	SetStr(pszStr);

    if (0 == strData.length())
	return true;

    //DisposeList();
    ErrorOccur = false;
    uiLookPos = 0;
    iLookAhead = lexan();

    expr();

    if (tokenBase.empty()) 
    {
	//THROW(new CParseException("No Data"));
	return false;
    }

    return !ErrorOccur;
}

void CPoly::Clear()
{
    int i;
    //while (!tokenBase.IsEmpty()) listBase.RemoveTail();
    //while (!numBase.IsEmpty()) numBase.RemoveTail();
    tokenBase.clear();
    numBase.clear();

    for (i = 0;i < STSize; ++i)
    {
	if (lSymbol[i]) delete lSymbol[i];
	lSymbol[i]=NULL;
    }
    //lSymbol.FreeExtra();
    lSymbol.clear();
    SymbolIndex.clear();
    STSize=0;
}

void CPoly::expr() 
{
    int t;

    switch (iLookAhead)
    {
	case '+':
	case '-':
	    uiLookPos--;
	    iLookAhead = NUM;
	    iNumToken = iToken = 0;

    }

    term();

    while (!ErrorOccur)
    {
	switch (iLookAhead)
	{
	    case '+':
	    case '-':
		t=iLookAhead;
		match(t);
		term();
		emit(t,NONE);
		continue;
	    case EOS: case ')': case ',': return;
	    default:
					  error();
					  //THROW( new CParseException("Error Parsing"));
					  return;
	}
    }
}

void CPoly::error()
{
    iErrorPos=uiLookPos;
    ErrorOccur=true;
}

int CPoly::lexan()
{
    int t;
    double tt;

    while (uiLookPos < strData.size())
    {
	if (strData[uiLookPos] == ' ' || strData[uiLookPos] == '\t')
	    ;
	else if (isdigit(strData[uiLookPos]))
	{
	    t = 0;
	    for (;uiLookPos<strData.size();uiLookPos++)
	    {
		if (isdigit(strData[uiLookPos]))
		    t = t * 10 + strData[uiLookPos] - '0';
		else
		    break;
	    }
	    iToken=t;
	    tt=0.1;
	    iNumToken=0;
	    if (uiLookPos<strData.size() && strData[uiLookPos]=='.')
	    {
		uiLookPos++;
		for (;uiLookPos<strData.size();uiLookPos++,tt*=0.1)
		{
		    if (isdigit(strData[uiLookPos]))
			iNumToken+=tt*(strData[uiLookPos]-'0');
		    else
			break;
		}
	    }
	    iNumToken+=iToken;
	    return NUM;
	}
	else if (isalpha(strData[uiLookPos]))
	{
	    string localSymbol("");
	    while (uiLookPos<strData.size() && isalpha(strData[uiLookPos]))
	    {
		localSymbol+=strData[uiLookPos];
		uiLookPos++;
	    }
	    iToken= find(localSymbol);
	    if (iToken==-1)
	    {
		iToken=insert(localSymbol,ID);
	    }
	    return lSymbol[(/*FindIndex*/(iToken))]->token;
	}
	else
	{
	    iToken=0;
	    return strData[uiLookPos++];
	}
	uiLookPos++;
    }
    return EOS;
}

void CPoly::term()
{
    int t;
    factor();
    while (!ErrorOccur)
    {
	switch (iLookAhead)
	{
	    case '*':
	    case '/':
	    case '%':
		t=iLookAhead;
		match(t);
		factor();
		emit(t,NONE);
		continue;
	    default:
		return;
	}
    }
}

void CPoly::factor()
{
    int t;
    expo();
    while (!ErrorOccur)
    {
	switch (iLookAhead)
	{
	    case '^':
		t=iLookAhead;
		match(t);
		expo();
		emit(t,NONE);
		continue;
	    default:
		return;
	}
    }
}

void CPoly::expo()
{
    int t;
    switch (iLookAhead)
    {
	case '(':
	    match('('); expr(); match(')'); break;
	case NUM:
	    emit(NUM, iToken); match(NUM); break;
	case ID:
	    emit(ID,(int)/*FindIndex*/(iToken)); match(ID); break;
	case ROOT:
	case SIN:
	case COT:
	case TAN:
	case CSC:
	case SEC:
	case LN:
	case LOG10:
	case COS:
	case ABS:
	case FLOOR:
	case SIGN:
	    t=iLookAhead;
	    match(iLookAhead); match('('); expr(); match(')'); emit(t,iToken);
	    break;

	case LOG:
	case MINF:
	case MAXF:
	case IRAND:
	case FRAND:
	case MOD:
	    t=iLookAhead;
	    match(iLookAhead); match('('); expr(); match(','); expr(); match(')'); emit(t,iToken);
	    break;
	case EOS:
	    break;
	default:
	    error();
	    //THROW( new CParseException("Error Parsing"));
    }
}

void CPoly::match(int t)
{
    if (iLookAhead==t) iLookAhead=lexan(); else error();
}

void CPoly::emit(int t, int tval)
{
    switch (t)
    {
	case '+': 
	    tokenBase.push_back(PLU);
	    break;
	case '-': 
	    tokenBase.push_back(MIN);
	    break;
	case '*': 
	    tokenBase.push_back(MUL);
	    break;
	case '/': 
	    tokenBase.push_back(DIV);
	    break;
	case '%':
	    tokenBase.push_back(MOD);
	    break;
	case '^':
	    tokenBase.push_back(POW);
	    break;
	case ROOT:
	case SIN:
	case TAN:
	case COT:
	case COS:
	case CSC:
	case SEC:
	case LOG:
	case LN:
	case LOG10:
	case ABS:
	case MINF:
	case MAXF:
	case IRAND:
	case FRAND:
	case MOD:
	case FLOOR:
	case SIGN:
	    tokenBase.push_back(t);
	    break;
	case NUM:
	    tokenBase.push_back(t);
	    numBase.push_back(iNumToken);
	    break;
	case ID:  
	    tokenBase.push_back(t);
	    tokenBase.push_back(tval); break;
	default:
	    error();
	    Clear();
	    //THROW( new CParseException("Error Parsing"));
	    return;
    }
}

int CPoly::find(const string & s)
{
	int l, m, r;

	l = 0;
	r = STSize - 1;

	while (l <= r)
	{
		m = (l + r) >> 1;

		if (lSymbol[SymbolIndex[m]]->strlex == s)
			return SymbolIndex[m];
		else if (lSymbol[SymbolIndex[m]]->strlex < s)
			l = m + 1;
		else
			r = m - 1;
	}
	return -1;
}

int CPoly::insert(const string & s, int tok)
{
	int i;
	bool bAdded=false;
	//s.MakeLower();
	//transform(s.begin(),s.end(),s.begin(),std::tolower);
	//lSymbol.SetAtGrow(STSize,new CSymTable(tok,s));
	lSymbol.push_back(new CSymTable(tok,s));
	for (i=0;i<STSize;i++)
	{
		if (s<lSymbol[SymbolIndex[i]]->strlex)
		{
			SymbolIndex.insert(SymbolIndex.begin()+i,STSize);
			bAdded=true;
			break;
		}
	}
	if (!bAdded)
	{
		//SymbolIndex.SetAtGrow(STSize,STSize);
		SymbolIndex.push_back(STSize);
	}
	STSize++;
	return STSize-1;
}

int CPoly::SetVar(const string & strName, double dVar)
{
    //transform(strName.begin(),strName.end(),s.begin(),std::tolower);
    if (ErrorOccur) return false;
    int index = find(strName);
    if (index == -1) return false;
    CSymTable * stVar = lSymbol[(/*FindIndex*/(index))];
    stVar->dVal = dVar;
    return true;
}

double CPoly::GetVar(const std::string & strName)
{
    if (ErrorOccur) return false;
    int index = find(strName);
    if (index == -1) return false;
    CSymTable * stVar = lSymbol[(/*FindIndex*/(index))];
	if(!stVar)
		return -1;
	return stVar->dVal;
}

void CPoly::init()
{
    insert("min",MINF);
    insert("max",MAXF);
    insert("number", IRAND);
    insert("irandom", IRAND);
    insert("irand", IRAND);
    insert("frandom",FRAND);
    insert("frand",FRAND);
    insert("rt",ROOT);
    insert("sqrt",ROOT);
    insert("cos",COS);
    insert("sin",SIN);
    insert("tan",TAN);
    insert("cot",COT);
    insert("csc",CSC);
    insert("cosec",COSEC);
    insert("sec",SEC);
    insert("pi",PI);
    SetVar("pi",3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068);
    insert("e",EXP);
    SetVar("e",2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427);
    insert("log",LOG);
    insert("ln",LN);
    insert("log10",LOG10);
    insert("abs",ABS);
    insert("mod",MOD);
    insert("floor", FLOOR);
    insert("sign",SIGN);
}

/*
   void CPoly::AddSymTable(CPtrList* ns)
   {
   int i, len = ns->GetCount();
   CSymTable* a, *b;
   int index;
   POSITION pos;
   pos = ns->GetHeadPosition();
   for (i=0;i<len;i++)
   {
   a = ((CSymTable*)ns->GetNext(pos));
   if ((index=find(a->strlex))==-1)
   {
   lSymbol[insert(a->strlex,a->token)]->dVal = a->dVal;
//lSymbol.SetAtGrow(STSize,b);
//STSize++;
}
else
{
(lSymbol.GetAt(FindIndex(index)))->dVal = a->dVal;
}

}
}
 */

/*
   void CPoly::Diff(CString& str)
   {
   int stNow, cc;
   int i;
   CString* mstr,*bstr;
   if (ErrorOccur) //THROW(new CEvalException("Diff Error"));
   return;
   mstr = new CString[tokenBase.GetCount()];
   bstr = new CString[tokenBase.GetCount()];
   POSITION pos = tokenBase.GetHeadPosition();
   POSITION posn = numBase.GetHeadPosition();
   cc=0;
   while (pos != NULL)
   {

   stNow=tokenBase.GetNext(pos);
   switch (stNow)
   {
   case NUM:
//save[iSp++]=numBase.GetNext(posn); 
mstr[cc]="0";bstr[cc].Format("%f",numBase.GetNext(posn)); 
for (i=bstr[cc].GetLength()-1;i>=0;i--)
if (bstr[cc][i] != '0')
break;
if (i>0)
bstr[cc]=bstr[cc].Left(i+1);
i=bstr[cc].GetLength()-1;
if (bstr[cc][i]=='.')
{
bstr[cc]=bstr[cc].Left(i);
}
cc++; break;

case ID:
////save[iSp++]=
//lSymbol.GetAt(
//lSymbol.FindIndex(
//(POSITION )tokenBase.GetNext(pos)
//				)
//)->dVal; 
bstr[cc] = lSymbol.GetAt(tokenBase.GetNext(pos))->strlex;
if (bstr[cc] != "x") mstr[cc]="0";
else  mstr[cc] = "1";
cc++;
break;
case '+':
cc--;
if (bstr[cc-1]=="0")
{
if (bstr[cc]=="0")
bstr[cc-1]="0"; 
else 
bstr[cc-1]="("+bstr[cc]+")";
}
else if (bstr[cc]=="0")
{
bstr[cc-1]="("+bstr[cc-1]+")";
}
else
bstr[cc-1]="("+bstr[cc-1]+"+"+bstr[cc]+")"; 
bstr[cc]="";

if (mstr[cc-1]=="0")
{
if (mstr[cc]=="0")
mstr[cc-1]="0"; 
else 
mstr[cc-1]="("+mstr[cc]+")";
}
else if (mstr[cc]=="0")
{
mstr[cc-1]="("+mstr[cc-1]+")";
}
else
mstr[cc-1]="("+mstr[cc-1]+"+"+mstr[cc]+")"; 
mstr[cc]="";

break;
case '-':
cc--;
if (bstr[cc-1]=="0")
{
    if (bstr[cc]=="0")
	bstr[cc-1]="0"; 
    else 
	bstr[cc-1]="(-"+bstr[cc]+")";
}
else if (bstr[cc]=="0")
{
    bstr[cc-1]="("+bstr[cc-1]+")";
}
else
bstr[cc-1]="("+bstr[cc-1]+"-"+bstr[cc]+")"; 
bstr[cc]="";

if (mstr[cc-1]=="0")
{
    if (mstr[cc]=="0")
	mstr[cc-1]="0"; 
    else 
	mstr[cc-1]="(-"+mstr[cc]+")";
}
else if (mstr[cc]=="0")
{
    mstr[cc-1]="("+mstr[cc-1]+")";
}
else
mstr[cc-1]="("+mstr[cc-1]+"-"+mstr[cc]+")"; 
mstr[cc]="";

break;
case '*':
cc--;
if (mstr[cc-1]=="0" || bstr[cc]=="0")
{
    if (mstr[cc]=="0" || bstr[cc-1]=="0")
    {
	mstr[cc-1]="0";
    }
    else
    {
	mstr[cc-1]="("+ mstr[cc] + "*" +bstr[cc-1] +")";
    }
}
else if (mstr[cc]=="0" || bstr[cc-1]=="0")
{
    mstr[cc-1] = "("+mstr[cc-1] + "*" + bstr[cc] + ")"; 
}
else
mstr[cc-1] = "(("+mstr[cc-1] + "*" + bstr[cc] + ")+(" +mstr[cc] + "*" +bstr[cc-1] +"))"; 

if (bstr[cc-1]=="0" || bstr[cc]=="0")
{
    bstr[cc-1]="0";
}
else
bstr[cc-1]="("+bstr[cc-1]+"*"+bstr[cc]+")"; 
mstr[cc]=""; bstr[cc]="";
break;
case '/':
cc--;
if (bstr[cc]=="0") THROW(new CEvalException("Divide by Zero"));
if (mstr[cc-1]=="0"||bstr[cc]=="0")
{
    if (mstr[cc]=="0" || bstr[cc-1]=="0")
    {
	mstr[cc-1]="0";
    }
    else
	mstr[cc-1]="((-" +mstr[cc]+"*"+bstr[cc-1]+ ")/("+bstr[cc]+")^2)";
}
else if (mstr[cc]=="0"||bstr[cc-1]=="0")
{
    mstr[cc-1]="((" +mstr[cc-1]+"*"+bstr[cc] + ")/("+bstr[cc]+")^2)";
}
else
mstr[cc-1]="((" +mstr[cc-1]+"*"+bstr[cc]+ "-" +mstr[cc]+"*"+bstr[cc-1]+ ")/("+bstr[cc]+")^2)";
if (bstr[cc-1]=="0")
bstr[cc-1]="0";
else
bstr[cc-1]="("+bstr[cc-1]+")/("+bstr[cc]+")";
break;
case '^':
cc--;
if (mstr[cc-1]=="0")
{
    if (mstr[cc]=="0")
    {
	mstr[cc-1]="0";
	mstr[cc]="";
    }
    else
    {
	mstr[cc-1]="ln("+bstr[cc-1]+")*("+bstr[cc-1]+")^("+bstr[cc]+")*("+mstr[cc]+")";
	mstr[cc]="";
    }
}
else if (mstr[cc]=="0")
{
    mstr[cc-1]="("+bstr[cc]+")*("+bstr[cc-1]+")^("+bstr[cc]+"-1)*("+mstr[cc-1]+")";
    mstr[cc]="";
}
else
{
    mstr[cc-1]="(ln("+bstr[cc-1]+")*("+bstr[cc-1]+")^("+bstr[cc]+")*("+mstr[cc]+")+("+bstr[cc]+")*("+bstr[cc-1]+")^("+bstr[cc]+"-1)*("+mstr[cc-1]+"))";
    mstr[cc]="";

}

if (bstr[cc-1]=="0")
{
    bstr[cc-1]="0";
}
else if (bstr[cc-1]=="1")
{
    bstr[cc-1]="1";
}
else if (bstr[cc]=="1")
{

}
else if (bstr[cc]=="0")
{
    bstr[cc-1]="0";
}
else
bstr[cc-1]="("+bstr[cc-1]+")^("+bstr[cc]+")";

bstr[cc]="";
break;
case ROOT:
cc--;
if (mstr[cc]=="0") 
mstr[cc]="0";
else
mstr[cc]="("+mstr[cc]+"/2/rt("+bstr[cc]+"))";
bstr[cc]="rt("+bstr[cc] +")";
cc++;
break;
case COS:
cc--;
mstr[cc]="(-"+mstr[cc]+"*sin("+bstr[cc]+"))";
bstr[cc]="cos("+bstr[cc]+")";
cc++;
break;
case SIN:
cc--;
mstr[cc]="("+mstr[cc]+"*cos("+bstr[cc]+"))";
bstr[cc]="sin("+bstr[cc]+")";
cc++;
break;
case TAN:
cc--;
mstr[cc]="(sec("+bstr[cc]+")^2*"+mstr[cc]+")";
bstr[cc]="tan("+bstr[cc]+")";

cc++;
break;
case CSC:
cc--;
mstr[cc]="(-csc("+bstr[cc]+")*cot("+bstr[cc]+")*"+mstr[cc]+")";
bstr[cc]="csc("+bstr[cc]+")";
cc++;
break;
case SEC:
cc--;
mstr[cc]="(sec("+bstr[cc]+")*tan("+bstr[cc]+")*"+mstr[cc]+")";
bstr[cc]="sec("+bstr[cc]+")";
cc++;
break;
case COT:
cc--;
mstr[cc]="(-csc("+bstr[cc]+")^2*"+mstr[cc]+")";
bstr[cc]="cot("+bstr[cc]+")";

cc++;
case LN:
cc--;
mstr[cc]="("+mstr[cc]+"/"+bstr[cc]+")";
bstr[cc]="ln("+bstr[cc]+")";
cc++;
break;
case LOG10:
cc--;
mstr[cc]="("+mstr[cc]+"/"+bstr[cc]+"/ln(10))";
bstr[cc]="log10("+bstr[cc]+")";
cc++;
break;
case LOG:
cc--;
mstr[cc-1]="("+mstr[cc]+"/"+bstr[cc]+"/ln("+bstr[cc-1]+")-ln("+bstr[cc]+")*"+mstr[cc-1]+"/ln("+bstr[cc-1]+")^2/"+bstr[cc-1]+")";
bstr[cc-1]="log("+bstr[cc-1]+","+bstr[cc]+")";
mstr[cc]="";
bstr[cc]="";

break;
default:
//THROW(new CEvalException("Out of Case"));
break;
}
}
str=mstr[cc-1];
delete[] mstr;
delete[] bstr;
}*/

/*int CPoly::FindIndex(int tval)
  {
  return tval;
  }
 */
