#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef MLOG_H
#define MLOG_H

#ifdef LOGGING
#define mlog(path, ...) {\
	char *logline = NULL; \
	asprintf(&logline, __VA_ARGS__); \
	write_log((path), logline); \
	free(logline); }
#else
#define mlog(...) {}
#endif

void write_log(char *path, char *msg);

#endif
