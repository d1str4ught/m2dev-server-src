#include <stdio.h>
#include <stdlib.h>

#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif

void WriteVersion()
{
#ifndef OS_WINDOWS
	FILE* fp(fopen("VERSION.txt", "w"));

	if (NULL != fp)
	{
		fprintf(fp, "db revision: %s\n", GIT_DESCRIBE);
		//fprintf(fp, "%s@%s:%s\n", __USER__, __HOSTNAME__, __PWD__);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot open VERSION.txt\n");
		exit(0);
	}
#endif
}

