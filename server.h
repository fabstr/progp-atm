/**
 * @file server.h
 * @brief The server program.
 */
#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "protocol.h"
#include "serverdb.h"
#include "mlog.h"

/**
 * @brief Handle a connection.
 *
 * Pass the Message through to handle_normal or handle_upgrade.
 * Handle the connection in a loop until a close_connection is sent by the
 * client, then return.
 *
 * @param socket The socket the message was received on.
 */
void handle_connection(int socket);

/**
 * @brief Handle an upgrade message.
 *
 * This function currently does nothing.
 *
 * @param socket The connection to the atm
 * @param m The message that was sent
 */
void handle_upgrade(int socket, Message *m);
	
/**
 * @brief Handle a normal atm to server message.
 * @param socket The connection to the atm
 * @param m The message that was sent
 */
void handle_normal(int socket, Message *m);
#endif /* SERVER_H */
