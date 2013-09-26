#include "serverdb.h"

sqlite3 *db = NULL;

sqlite3_stmt *balance_stmt = NULL;
sqlite3_stmt *update_stmt = NULL;
sqlite3_stmt *insert_stmt = NULL;
sqlite3_stmt *get_next_otp_stmt = NULL;
sqlite3_stmt *update_otp_stmt = NULL;

char *create_table_string = "CREATE TABLE accounts("
	"id INTEGER PRIMARY KEY AUTOINCREMENT, cardnumber INTEGER, "
	"pinhash TEXT, balance INTEGER, onetimekey INTEGER DEFAULT 1);";

char *balance_string = "SELECT balance FROM accounts "
	"WHERE cardnumber LIKE :cardnumber AND pinhash LIKE :pin";

char *update_string = "UPDATE accounts "
	"SET balance = :newbalance "
	"WHERE cardnumber LIKE :cardnumber "
	"AND pinhash LIKE :pin";

char *insert_string = "INSERT INTO accounts (cardnumber, pinhash, balance) "
	"VALUES (?1, ?2, ?3)";

char *get_next_otp_string = "SELECT onetimekey FROM accounts "
	"WHERE cardnumber LIKE :cardnumber AND pinhash LIKE :pin";

char *update_otp_string = "UPDATE accounts SET onetimekey = onetimekey + 2 "
	"WHERE cardnumber LIKE :cardnumber AND pinhash LIKE :pin";

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
	res = sqlite3_prepare_v2(db, balance_string, strlen(balance_string)+1, 
		&balance_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s", 
				balance_string, sqlite3_errmsg(db));
		return 1;
	}

	res = sqlite3_prepare_v2(db, update_string, strlen(update_string)+1, 
			&update_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				update_string, sqlite3_errmsg(db));
		return 1;
	}

	res = sqlite3_prepare_v2(db, insert_string, strlen(insert_string)+1, 
			&insert_stmt, &unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				insert_string, sqlite3_errmsg(db));
		return 1;
	}

	res = sqlite3_prepare_v2(db, get_next_otp_string, 
			strlen(get_next_otp_string)+1, &get_next_otp_stmt, 
			&unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				get_next_otp_string, sqlite3_errmsg(db));
		return 1;
	}

	res = sqlite3_prepare_v2(db, update_otp_string, 
			strlen(update_otp_string)+1, &update_otp_stmt, 
			&unused);
	if (res != SQLITE_OK) {
		mlog("server.log", "Could not prepare ('%s'): %s",
				 update_otp_string, sqlite3_errmsg(db));
		return 1;
	}
	
	return 0;
}

int close_db()
{
	sqlite3_finalize(balance_stmt);
	sqlite3_finalize(update_stmt);
	sqlite3_finalize(insert_stmt);
	return sqlite3_close(db);
}

/**
 * Return the balance for the user specified in message
 * Return 0 on error, else 1.
 * @param m The message holding card number and pin
 * @param balance (OUT) An integer to hold the balance.
 */
int getBalance(Message *m, uint16_t *balance)
{
	mlog("server.log", "getting balance for card # %d", m->card_number);
	int toReturn = 0;

	char *texthash = NULL;
	asprintf(&texthash, "%d", m->pin);
	int texthashlen = strlen(texthash);
	
	*balance = 4;

	if (texthash == NULL) {
		mlog("server.log", "Could not allocate memory for texthash.");
		return 1;
	} else if (sqlite3_bind_int(balance_stmt, 1, m->card_number) != SQLITE_OK) {
		mlog("server.log", "Could not bind card number.");
		toReturn = 1;
	} else if (sqlite3_bind_text(balance_stmt, 2, texthash, texthashlen, 
				SQLITE_TRANSIENT) != SQLITE_OK) {
		mlog("server.log", "Could not bind pin hash.");
		toReturn = 1;
	} else {
		/* the statement is prepared */
		int looping = 1;
		int results = 0;
		while (looping) {
			int res = sqlite3_step(balance_stmt);
			if (res == SQLITE_BUSY) {
				sleep(10);
				continue;
				break;
			} else if (res == SQLITE_DONE) {
				break;
			} else if (res == SQLITE_ROW) {
				results++;
				*balance = sqlite3_column_int(balance_stmt, 0);
				break;
			} else {
				free(texthash);
				mlog("server.log", "Could not step: %s", 
						sqlite3_errmsg(db));
				return 1;
			}
		}

		if (results == 0) {
			m->message_id = no;
		}

		mlog("server.log", "got balance %d for %d", *balance, m->card_number);
	}

	sqlite3_reset(balance_stmt);
	free(texthash);
	return toReturn;
}

int insert(Message *m)
{
	int toReturn = 0;
	char *texthash = NULL;
	asprintf(&texthash, "%d", m->pin);
	int texthashlen = strlen(texthash);

	if (texthash == NULL) {
		mlog("server.log", "Could not allocate memory for texthash.");
		return 1;
	} else if (sqlite3_bind_int(insert_stmt, 1, m->card_number) != SQLITE_OK) {
		mlog("server.log", "Could not bind card number: %s", 
				sqlite3_errmsg(db));
		toReturn = 1;
	} else if (sqlite3_bind_text(insert_stmt, 2, texthash, texthashlen, 
				SQLITE_TRANSIENT) != SQLITE_OK) {
		mlog("server.log", "Could not bind pin hash.");
		toReturn = 1;
	} else if (sqlite3_bind_int(insert_stmt, 3, m->sum) != SQLITE_OK) {
		mlog("server.log", "Could not bind sum.");
		toReturn = 1;
	} else {
		/* the statement is prepared */
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
				free(texthash);
				mlog("server.log", "Could not step: %s", 
						sqlite3_errmsg(db));
				return 1;
			}
		}
	}

	sqlite3_reset(insert_stmt);
	free(texthash);
	return toReturn;
}

int update(Message *m)
{
	int toReturn = 0;

	char *texthash = NULL;
	asprintf(&texthash, "%d", m->pin);
	int texthashlen = strlen(texthash);

	if (texthash == NULL) {
		mlog("server.log", "Could not allocate memory for texthash.");
		return 1;
	} else if (sqlite3_bind_int(update_stmt, 1, m->sum) != SQLITE_OK) {
		mlog("server.log", "Could not bind sum.");
		toReturn = 1;
	} else if (sqlite3_bind_int(update_stmt, 2, m->card_number) != SQLITE_OK) {
		mlog("server.log", "Could not bind card number.");
		toReturn = 1;
	} else if (sqlite3_bind_text(update_stmt, 3, texthash, texthashlen, 
				SQLITE_TRANSIENT) != SQLITE_OK) {
		mlog("server.log", "Could not bind pin hash.");
		toReturn = 1;
	} else {
		/* the statement is prepared */
		int res;
		while ((res = sqlite3_step(update_stmt)) != SQLITE_DONE) {
			switch (res) {
			case SQLITE_BUSY:
				sleep(10);
				continue;
				break;
			case SQLITE_ERROR:
			case SQLITE_MISUSE:
			default:
				free(texthash);
				mlog("server.log", "Could not step: %s", 
						sqlite3_errmsg(db));
				return 1;
			}
		}
	}

	sqlite3_reset(update_stmt);
	free(texthash);
	return toReturn;
}

int updateOnetimekey(Message *m)
{
	int toReturn = 0;

	char *texthash = NULL;
	asprintf(&texthash, "%d", m->pin);
	int texthashlen = strlen(texthash);

	if (texthash == NULL) {
		mlog("server.log", "Could not allocate memory for texthash.");
		return 1;
	} else if (sqlite3_bind_int(update_otp_stmt, 1, m->card_number) != SQLITE_OK) {
		mlog("server.log", "Could not bind card number.");
		toReturn = 1;
	} else if (sqlite3_bind_text(update_otp_stmt, 2, texthash, texthashlen, 
				SQLITE_TRANSIENT) != SQLITE_OK) {
		mlog("server.log", "Could not bind pin hash.");
		toReturn = 1;
	} else {
		/* the statement is prepared */
		int res;
		while ((res = sqlite3_step(update_otp_stmt)) != SQLITE_DONE) {
			switch (res) {
			case SQLITE_BUSY:
				sleep(10);
				continue;
				break;
			case SQLITE_ERROR:
			case SQLITE_MISUSE:
			default:
				free(texthash);
				mlog("server.log", "Could not step: %s", 
						sqlite3_errmsg(db));
				return 1;
			}
		}
	}

	sqlite3_reset(update_otp_stmt);
	free(texthash);
	return toReturn;
}

int getNextOnetimekey(Message *m)
{
	mlog("server.log", "getting otk for card # %d", m->card_number);
	int toReturn = 0;

	char *texthash = NULL;
	asprintf(&texthash, "%d", m->pin);
	int texthashlen = strlen(texthash);
	
	if (texthash == NULL) {
		mlog("server.log", "Could not allocate memory for texthash.");
		return 1;
	} else if (sqlite3_bind_int(get_next_otp_stmt, 1, m->card_number)
			!= SQLITE_OK) {
		mlog("server.log", "Could not bind card number.");
		toReturn = 1;
	} else if (sqlite3_bind_text(get_next_otp_stmt, 2, texthash,
				texthashlen, SQLITE_TRANSIENT) != SQLITE_OK) {
		mlog("server.log", "Could not bind pin hash.");
		toReturn = 1;
	} else {
		/* the statement is prepared */
		int looping = 1;
		int results = 0;
		while (looping) {
			int res = sqlite3_step(get_next_otp_stmt);
			if (res == SQLITE_BUSY) {
				sleep(10);
				continue;
				break;
			} else if (res == SQLITE_DONE) {
				break;
			} else if (res == SQLITE_ROW) {
				results++;
				int r = sqlite3_column_int(
						get_next_otp_stmt, 0);
				mlog("server.log", "got res=%d", r);
				m->onetimecode = (uint8_t) r;
				break;
			} else {
				free(texthash);
				mlog("server.log", "Could not step: %s", 
						sqlite3_errmsg(db));
				return 1;
			}
		}

		if (results == 0) {
			m->message_id = no;
		}

		mlog("server.log", "got otk %d for %d", m->onetimecode,
				m->card_number);
	}

	sqlite3_reset(get_next_otp_stmt);
	free(texthash);
	return toReturn;
}
