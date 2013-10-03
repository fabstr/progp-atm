/**
 * @file client.h
 * @brief The atm client.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>

#include "mlog.h"
#include "protocol.h"
#include "clientdb.h"


/**
 * @brief Show the user's balance.
 *
 * Ask the server for the balance via socket and specify the credentials in the
 * message.
 *
 * @param socket The socket to send messages on
 * @param c The credentials of the user.
 */
void show_balance(BIO *bio, Credentials *c);

/**
 * @brief Deposit money for the user.
 *
 * @param socket The socket to send data on.
 * @param c The credentials of the user.
 */
void deposit_money(BIO *bio, Credentials *c);

/**
 * @brief Withdraw money for the user.
 * otp must be set in the Credentials.
 * @param socket The socket to send data on.
 * @param c The credentials of the user.
 */
void withdraw_money(BIO *bio, Credentials *c);

/**
 * @brief Ask the user for credentials.
 * 
 * The user will be prompted for card number and pin code.  The pin code is 
 * gotten with getpass, there will be no echoing of the pin code.
 *
 * @param target The memory block where to store the credentials.
 * @return 0 on success.
 */
int getCredentials(Credentials *target);

/**
 * @brief Ask the user for an integer, printing prompt.
 * 
 * This function does not return until sscanf(3) have read an integer.
 *
 * @param prompt The prompt to show the user
 * @return The integer.
 */
uint16_t askForInteger(char *prompt);

/**
 * @brief Handle an incoming message.
 *
 * Depending on the message type it will be passed through to welcome_update or
 * language_add.
 *
 * @param socket The socket to receive data on.
 */
void upgrade_handle(BIO *bio);

/**
 * @brief The main loop for the client
 *
 * Ask the user for commands, and execute them accordingly.
 *
 * By writing "poweroff", the client process is gracefully exited. The user can
 * change language by writing "change language".
 *
 * @param socket The socket file descriptor of the connection to the server.
 */
void start_loop(BIO *bio);

/**
 * @brief Update the welcome message.
 *
 * Expect two network strings: one that specifies the language code, the other
 * with the actual welcome message.
 *
 * @param socket The socket to receive data on
 * @param m The message that says two network strings are to be expected.
 */
void update_welcome(BIO *bio, Message *m);

/**
 * @brief Add a language to the client.
 *
 * With m being a language_add message, listen for the correct amount of network
 * strings and call the database to add the language.
 *
 * The network strings should follow the order they appear in queryStrings in 
 * manage.c.
 *
 * The function will not return untill all the network strings have been 
 * received.
 *
 * @param socket The socket to recieve data on
 * @param m The message that specifies the number of network strings to expect.a
 */
void add_language(BIO *bio, Message *m);

/**
 * @brief Function to start a thread for the upgrade server.
 *
 * Start the server at UPGRPORT, calling upgrade_handle for each connection and
 * listen on (the global variable) upgrade_socket.
 *
 * @param arg Not used.
 * @return Not used.
 */
void *server_thread(void *arg);

/**
 * @brief Start a dialog with the user and change the language.
 *
 * The user is presented with a list of languages and chooses a language code.
 * setLanguage is the called.
 */
void changeLanguage();

/** 
 * @brief Set the language code.
 * @param code The new language code.
 */
void setLanguage(char *code);

#endif /* CLIENT_H */
