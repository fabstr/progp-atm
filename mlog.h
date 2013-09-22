#ifndef MLOG_H
#define MLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef LOGGING
#define mlog(path, ...) {\
	char *logline = NULL; \
	asprintf(&logline, __VA_ARGS__); \
	write_log((path), logline, __LINE__, __FILE__); \
	free(logline); }
#else
#define mlog(...) {}
#endif

void write_log(char *path, char *msg, int line, char *file);

#endif
