/**
 * @file manage.h
 * @brief A managing program to update the clients.
 */
#ifndef MANAGE_H
#define MANAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "protocol.h"

/**
 * @brief Query the user for strings in the language and send the to a host.
 *
 * The host will be queried for.
 *
 * @return 0 on success or 1 on failure.
 */
int addLanguage();

/**
 * @brief Change the welcome message.
 *
 * Ask for a host, a language code and the welcome message.
 *
 * @return 0 0 success or 1 on failure.
 */
int changeWelcome();

/**
 * @brief Print a help message.
 */
void printHelp();

#endif /* MANAGE_H */
