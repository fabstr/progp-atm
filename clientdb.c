#include "clientdb.h"

#define DBPATH "client.sqlite"

const char *create_table_string = "CREATE TABLE languages("
	"id INTEGER PRIMARY KEY AUTOINCREMENT, langcode TEXT, langname TEXT,"
	"cmd_quit TEXT, cmd_balance TEXT, cmd_deposit TEXT, cmd_withdraw TEXT,"
	"cmd_help TEXT, error_unknown_command TEXT, error_balance TEXT,"
	"error_deposit TEXT, error_withdraw TEXT, msg_welcome TEXT,"
	"msg_balance TEXT, msg_deposit TEXT, msg_withdraw TEXT,"
	"rqst_enter_amount TEXT, rqst_enter_otp TEXT, rqst_card_number TEXT,"
	"rqst_pin TEXT)";

const char *get_string = "SELECT ?1 FROM languages WHERE langcode LIKE ?2";

const char *insert_string = "INSERT INTO languages (langcode, langname,"
	"cmd_quit, cmd_balance, cmd_deposit, cmd_withdraw, cmd_help,"
	"error_unknown_command, error_balance, error_deposit, error_withdraw,"
	"msg_welcome, msg_balance, msg_deposit, msg_withdraw, rqst_enter_amount,"
	"rqst_enter_otp, rqst_card_number, rqst_pin)"
	"VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, "
	"?15, ?16, ?17, ?18, ?19)";

const char *update_welcome_string = "UPDATE languages "
	"SET msg_welcome = ?1 "
	"WHERE langcode LIKE ?2";

sqlite3 *db = NULL;

sqlite3_stmt *create_table_stmt = NULL;
sqlite3_stmt *get_string_stmt = NULL;
sqlite3_stmt *insert_stmt = NULL;
sqlite3_stmt *update_welcome_stmt = NULL;

/**
 * Initialise the database and the statements.
 * If there is an error, print it to stderr.
 * @return 0 on success or 1 on error.
 */
int setup_db()
{
	int res = sqlite3_open(DBPATH, &db);
	if (res != 0) {
		mlog("server.log", "Could not open database: %s",
				sqlite3_errmsg(db));
		return 1;
	}
	
	const char *unused;

	/*res = sqlite3_prepare_v2(db, create_table_string, strlen(create_table_string)+1, 
			&create_table_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				create_table_string, sqlite3_errmsg(db));
		close_db();
		return 1;
	}*/

	res = sqlite3_prepare_v2(db, get_string, strlen(get_string)+1, 
		&get_string_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s", 
				get_string, sqlite3_errmsg(db));
		close_db();
		return 1;
	}

	res = sqlite3_prepare_v2(db, insert_string, strlen(insert_string)+1, 
			&insert_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				insert_string, sqlite3_errmsg(db));
		close_db();
		return 1;
	}
	
	res = sqlite3_prepare_v2(db, update_welcome_string, 
			strlen(update_welcome_string)+1, &update_welcome_stmt, 
			&unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				update_welcome_string, sqlite3_errmsg(db));
		close_db();
		return 1;
	}
	return 0;
}

int close_db()
{
	sqlite3_finalize(create_table_stmt);
	sqlite3_finalize(get_string_stmt);
	sqlite3_finalize(insert_stmt);
	sqlite3_finalize(update_welcome_stmt);
	return sqlite3_close(db);
}

char *getString(Strings string_name, char *lang_code)
{
	char *stringName = string_from_enum(string_name);
	char *string = NULL;
	int results = 0;
	
	if (sqlite3_bind_text(get_string_stmt, 1, stringName, strlen(stringName),
				SQLITE_STATIC) != SQLITE_OK) {
		mlog("client.log", "Could not bind column name: %s",
				sqlite3_errmsg(db));
	} else if (sqlite3_bind_text(get_string_stmt, 2, lang_code, 3,
				SQLITE_STATIC) != SQLITE_OK) {
		mlog("client.log", "Could not bind language code: %s",
				sqlite3_errmsg(db));
	}
	while (1) {
		int res = sqlite3_step(get_string_stmt);
		if (res == SQLITE_BUSY) {
			sleep(10);
			continue;
		} else if (res == SQLITE_DONE) {
			break;
		} else if (res == SQLITE_ROW) {
			results++;
			string = (char *) sqlite3_column_text(get_string_stmt, 0);
		} else {
			mlog("client.log", "Could not step: %s",
					sqlite3_errmsg(db));
			return NULL;
		}
	}

	sqlite3_reset(get_string_stmt);
	return string;
}

int create_language(char **strings, int count)
{
	if (count != 19) {
		mlog("client.log", "want 19 strings for a new language, got %d",
				count);
		return 1;
	}

	int i;
	for (i=1; i<=19; i++) {
		if (sqlite3_bind_text(insert_stmt, 
					i, 
					strings[i-1], 
					strlen(strings[i-1])+1, 
					SQLITE_TRANSIENT) != SQLITE_OK) {
			mlog("client.log", "could not bind text: %s", 
				sqlite3_errmsg(db));
			return 1;
		}
	}

	int res;
	while ((res = sqlite3_step(insert_stmt)) != SQLITE_DONE) {
		switch (res) {
		case SQLITE_BUSY:
			sleep(10);
			continue;
			break;
		case SQLITE_ERROR:
		case SQLITE_MISUSE:
		default:
			mlog("client.log", "Could not step: %s", 
					sqlite3_errmsg(db));
			return 1;
			break;
		}
	}

	sqlite3_reset(insert_stmt);
	return 0;
}

int update_welcome_db(char *lang_code, char *newmsg)
{
	if (sqlite3_bind_text(update_welcome_stmt, 1, newmsg, strlen(newmsg)+1, 
		SQLITE_STATIC) != SQLITE_OK) {
		mlog("client.log", "could not bind welcome message: %s", 
				sqlite3_errmsg(db));
		return 1;
	} else if (sqlite3_bind_text(update_welcome_stmt, 2, lang_code, 
				strlen(lang_code)+1, SQLITE_STATIC) 
			!= SQLITE_OK) {
		mlog("client.log", "could not bind language code: %s", 
				sqlite3_errmsg(db));
		return 1;
	}

	int res;
	while ((res = sqlite3_step(update_welcome_stmt)) != SQLITE_DONE) {
		switch (res) {
		case SQLITE_BUSY:
			sleep(10);
			continue;
			break;
		case SQLITE_ERROR:
		case SQLITE_MISUSE:
		default:
			mlog("client.log", "Could not step: %s", 
					sqlite3_errmsg(db));
			return 1;
			break;
		}
	}

	sqlite3_reset(update_welcome_stmt);
	return 0;
}

char *string_from_enum(Strings string_name)
{
	switch (string_name) {
	case cmd_quit:
		return "cmd_quit"; 
		break;
	case cmd_balance:
		return "cmd_balance"; 
		break;
	case cmd_deposit:
		return "cmd_deposit"; 
		break;
	case cmd_withdraw:
		return "cmd_withdraw"; 
		break;
	case cmd_help:
		return "cmd_help"; 
		break;
	case error_unknown_command:
		return "error_unknown_command"; 
		break;
	case error_balance:
		return "error_balance"; 
		break;
	case error_deposit:
		return "error_deposit"; 
		break;
	case error_withdraw:
		return "error_withdraw"; 
		break;
	case msg_welcome:
		return "msg_welcome"; 
		break;
	case msg_balance:
		return "msg_balance"; 
		break;
	case msg_deposit:
		return "msg_deposit"; 
		break;
	case msg_withdraw:
		return "msg_withdraw"; 
		break;
	default:
		return NULL;
		break;
	}
}
