#include "stdafx.h"
#include "affect.h"

CAffect* CAffect::Acquire()
{
	return new CAffect;
}

void CAffect::Release(CAffect* p)
{
	delete p;
}

