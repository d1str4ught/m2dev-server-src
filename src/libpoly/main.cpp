#ifdef __WIN32__
#include <windows.h>
#endif

#include "Poly.h"

#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace std;

int main(int argc, char ** argv)
{
	printf( "12345\n" );

#ifndef __WIN32__
	srandom(time(0) + getpid());
#else
	srand(time(0) + GetCurrentProcessId());
#endif

    if (argc != 3)
	{
		cout << "argc != 3"  << endl;
		return 0;
	}

    CPoly p;
    p.SetStr(argv[1]); // "min(5,max(-5,(x-y+10)))+min(5,max(-5,(x-y+10)))"

    if (!p.Analyze())
    {
	cout << "Analyze failed" << endl;
	return 0;
    }

    printf("B = %d\n", atoi(argv[2]));

    p.SetVar("b", atoi(argv[2]));
    p.SetVar("k", 20);

    cout << (int) p.Eval() << endl;
    cout << p.Eval() << endl;
    return 0;
}

