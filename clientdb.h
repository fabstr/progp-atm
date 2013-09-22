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

typedef enum {
	/* commands the user writes */
	cmd_quit = 3, cmd_balance, cmd_deposit, cmd_withdraw, cmd_help, 

	/* error messages */
	error_unknown_command, error_balance, error_deposit, error_withdraw, 

	/* success messages */
	msg_welcome, msg_balance, msg_deposit, msg_withdraw,

	/* requests to the user */
	rqst_enter_amount, rqst_enter_otp, rqst_card_number, rqst_pin,

	/* values added after initial sql queries */
	cmd_change_lang,
} Strings;

int setup_db();
int close_db();

char *getString(Strings string_name, char *lang_code);

int create_language(char **strings, int count);

int update_welcome_db(char *lang_code, char *newmsg);

char *string_from_enum(Strings string_name);

void printAvaliableLanguages();

bool correctLangCode(char *code);

#endif /* CLIENTDB_H */
