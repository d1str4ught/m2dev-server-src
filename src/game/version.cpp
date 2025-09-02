#include <stdio.h>

#ifndef GIT_DESCRIBE_VERSION
#define GIT_DESCRIBE_VERSION "unknown"
#endif

void WriteVersion()
{
#ifndef OS_WINDOWS
	FILE* fp = fopen("version.txt", "w");

	if (fp)
	{
		fprintf(fp, "game revision: %s\n", GIT_DESCRIBE_VERSION);
		//fprintf(fp, "game revision: %s\n", __SVN_VERSION__);
		//fprintf(fp, "%s@%s:%s\n", __USER__, __HOSTNAME__, __PWD__);
		fclose(fp);
	}
#endif
}

