#include "mlog.h"

void write_log(char *path, char *msg, int line, char *file)
{
	FILE *f = fopen(path, "a+b");
	
	/* get the current timestamp and remove the newline */
	time_t currTime = time(NULL);
	char *timeString = ctime(&currTime);
	timeString[24] = '\0';

	/* print the message and close the file */
	fprintf(f, "%s (%s:%d): %s \n", timeString, file, line, msg);
	fclose(f);
}
