/**
 * @file serverdb.h
 * @brief Databse function for the server program.
 */
#ifndef SERVERDB_H
#define SERVERDB_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "networking.h"
#include "mlog.h"
#include "sqlite3.h"

#define DBPATH "db.sqlite"

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
 * @brief Get the balance for a user.
 *
 * The user's credentials should be specified in the message and the balance
 * will be placed in balance.
 *
 * @param m The message with the user's credentials.
 * @param balance A pointer to the memory block where to store the balance.
 * @return 0 on success or 1 on failure.
 */
int getBalance(Message *m, uint16_t *balance);

/**
 * @brief Add a user to the database.
 *
 * @param m A message with the new user's credentials and money.
 * @return 0 on success or 1 on failure.
 */
int insert(Message *m);

/**
 * @brief Update an account.
 *
 * The user specified by m will have the account updated according to m->sum.
 *
 * @param m The message.
 */
int update(Message *m);

/**
 * @brief Increase the user's one time key.
 * @param m The message witht the user's credentials.
 * @return 0 on success or 1 on failure.
 */
int updateOnetimekey(Message *m);

/**
 * @brief Return the user's next one time key in the message.
 * @param m The message to return the one time key with.
 * @return 0 on success or 1 on failure.
 */
int getNextOnetimekey(Message *m);


#endif /* SERVERDB_H */
