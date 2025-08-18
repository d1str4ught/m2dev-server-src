#include "stdafx.h"
#include "affect.h"

CAffect* CAffect::Acquire()
{
	return M2_NEW CAffect;
}

void CAffect::Release(CAffect* p)
{
	M2_DELETE(p);
}

