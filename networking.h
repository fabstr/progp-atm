/**
 * @file networking.h
 * @brief Network-related stuff.
 *
 * networking.h defines functions for the client and server to talk to each other
 * over the internet.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>

#include <polarssl/net.h>

#include "mlog.h"

/** @brief port for normal client/server communication */
#define PORT "4567"

/** @brief port for manage/client upgrade communication */
#define UPGRPORT "4568"

/** @brief The number of pending connections for the server */
#define BACKLOG 10

/** @brief The size of the buffer when receiving a message */
#define BUFFSIZE 10

/**
 * @brief The valid message ids.
 */
typedef enum {
	/* "normal" messages */
	no = 0, balance, withdraw, deposit,

	/* update messages */
	atm_key, language_add, welcome_update,

	/* control messages */
	close_connection, add_account
} Message_ID;

/**
 * @brief The message that is used internally in the server and the client.
 */
typedef struct {
	/** @brief the message id */
	Message_ID message_id;

	/** @brief the sum, when applicable */
	uint16_t sum;

	/** @brief the pin code */
	uint16_t pin;

	/** @brief the one time code, when applicable */
	uint8_t onetimecode;

 	/** @brief the card number */
	uint16_t card_number;
} Message;

/**
 * @brief The message that is actually sent over the network.
 */
typedef struct {
	/** @brief the message id (converted from enum) */
	uint8_t message_id;

	/** @brief the sum, when applicable */
	uint8_t onetimecode;

	/** @brief the pin code */
	uint16_t sum;

	/** @brief the one time code, when applicable */
	uint16_t pin;

 	/** @brief the card number */
	uint16_t card_number;
} NetworkMessage;

/**
 * @brief The network string has a length and data, the length is sent in its own
 * package to let the receiver allocate memory.
 */
typedef struct {
	/** @brief length of the following string */
	uint8_t string_length;

        /** @brief the string */
	char *string; 
} NetworkString;
 
/**
 * @brief Credentials for a user. 
 * otp is not always applicable.
 */
typedef struct {
	/** @brief The user's card number */
	uint32_t card_number;

	/** @brief The user's pin code */
	uint16_t pin;

	/** @brief When needed, the user's current one time key*/
	uint8_t otp;
} Credentials;

/**
 * @brief Chose the correct socket address, for IPv4 or IPv6.
 * @param socketaddress 
 * @return The socket address, as sin_addr or sin6_addr.
 */
void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);

/**
 * @brief Receive a message via connection.
 *
 * Wait for and receive a network message via connection and place it in target. 
 * The network message is converted to a normal message and the integers to host
 * byte order. target should be a pointer to a memory area large enough to hold
 * a Message.
 *
 * @param connection The socket to receive data on.
 * @param target A pointer to a memory block where to store the message.
 * @return 0 on success, or -1 on failure.
 */
int getMessage(int connection, Message *target);

/**
 * @brief Send a message over connection
 *
 * Send the message via the connection socket. It will be converted to a 
 * network message, with all the integers in network byte order. message_id is
 * converted to an unsigned byte.
 *
 * @param connection The socket to send on
 * @param m The message to send
 * @return The number of bytes sent.
 */
size_t sendMessage(int connection, Message *msg);

/**
 * @brief Print a message to stdout.
 * 
 * @param m The message to print.
 */
void printMessage(Message *m);

/**
 * @brief Connect to a server and return the socket.
 *
 * Connect to the server specified by hostname and port. If all goes well,
 * return the socket file descriptor. Else return EXIT_FAILURE.
 *
 * @param hostname The hostname of the server to connect to.
 * @param port The port to connect on.
 * @return EXIT_FAILURE on failure, or a socket file descriptor on success.
 */
int connectToServer(char *hostname, char *port);

/**
 * @brief Send a network string.
 * 
 * Construct a networkstring from the string, then send it via socket. strlen(3)
 * is used to determine the length of the string.
 *
 * @param socket The socket to send the string on.
 * @param string The string to send.
 * @return 0 on sucess, else non-zero.
 */
int sendNetworkString(int socket, char *string);

/**
 * 
 */
void sigchld_handler(int s);

/**
 * @brief Start a server listening on port.
 *
 * Start a (stream) server that listens on the specified port, then calls
 * fork() for each connection. When the process is fork()'ed, call handle with
 * the accept()'ed socket.
 *
 * @param port The port to listen on.
 * @param handle A function to call for each connection. It will be passed the
 *               accept()'ed socket as its argument. The function should not
 *               return until the socket is to be closed.
 * @param sock A pointer to the storage for the socket file descriptor, this
 *             makes it possible to close the socket from another part of the 
 *             program.
 * @return EXIT_FAILURE on failure, it should not return on success.
 */
int start_server(char *port, void(*handle)(int), int *sock);

/**
 * @brief Receive a network string.
 *
 * Receive a network string from the (open) socket. The result is stored in 
 * nstr, which should have been initialized before calling this function.
 *
 * @param socket The socket to receive data on
 * @param nstr The network string to store the data in
 * @return 0 on success, else 1.
 */
int getNetworkString(int socket, NetworkString *nstr);


#endif /* PROTOCOL_H */
