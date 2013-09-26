/**
 * @file clientdb.h
 * @brief Database functions for client.
 */
#ifndef CLIENTDB_H
#define CLIENTDB_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "protocol.h"
#include "mlog.h"
#include "sqlite3.h"

/**
 * @brief 
 */
typedef enum {
	/* commands the user writes */
	cmd_quit = 3, cmd_balance, cmd_deposit, cmd_withdraw, cmd_help, 

	/* error messages */
	error_unknown_command, error_balance, error_deposit, error_withdraw, 

	/* success messages (and welcome and help) */
	msg_welcome, msg_balance, msg_deposit, msg_withdraw, msg_help,

	/* requests to the user */
	rqst_enter_amount, rqst_enter_otp, rqst_card_number, rqst_pin,

	/* values added after initial sql queries */
	cmd_change_lang,
} Strings;

/**
 * @brief Initialize the sqlite connections.
 * @return 0 on success or 1 on failure.
 */
int setup_db();

/**
 * @brief Close and finalize the sqlite connections.
 * @return The return value of sqlite3_close().
 */
int close_db();

/**
 * @brief Get a user interface string.
 * @param string_name The string's name.
 * @param lang_code The language code to use.
 * @return The string, or NULL on failure.
 */
char *getString(Strings string_name, char *lang_code);

/**
 * @brief Add a language.
 *
 * 20 ui strings are expected in the order they are defined in manage.c.
 *
 * @param strings An array of the strings.
 * @param The number of strings in the array.
 * @return 0 on success or 1 on failure.
 */
int create_language(char **strings, int count);

/**
 * @brief Update the welcome message in the database.
 * @param lang_code The language code of the message
 * @param newmsg The new welcome message.
 * @return 0 on success or 1 in failure.
 */
int update_welcome_db(char *lang_code, char *newmsg);

/**
 * @brief Return a string from the Strings enum.
 * 
 * For example, string_from_enum(cmd_quit) returns "cmd_quit".
 *
 * @param string_name The name of the string
 * @return The corresponding string.
 */
char *string_from_enum(Strings string_name);

/**
 * @brief Print a list of the available languages.
 *
 * The list will be printed to stdout.
 */
void printAvaliableLanguages();

/**
 * @brief Check the language code is correct.
 *
 * The language code is correct if code consists of two characters (ie strlen(3)
 * returns 2).
 *
 * @param code The language code to check
 * @return true if the code is correct, else false.
 */
bool correctLangCode(char *code);

#endif /* CLIENTDB_H */
