/**
 * @file mlog.h
 * @brief A simple logging macro.
 */
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

/**
 * @brief Write a log message.
 *
 * This function is intended to be used with the mlog() macro.
 *
 * The current time/date will also be printed.
 *
 * @param path The log file to write to
 * @param msg The message to write
 * @param line The line number in the file
 * @param file The file
 */
void write_log(char *path, char *msg, int line, char *file);

#endif
