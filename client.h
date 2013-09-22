#ifndef CLIENT_H
#define CLIENT_H

#include <readline/readline.h>
#include <readline/history.h>

#include <pthread.h>

#include "mlog.h"
#include "protocol.h"
#include "clientdb.h"

void show_balance(int socket, Credentials *c);
void deposit_money(int socket, Credentials *c);
void withdraw_money(int socket, Credentials *c);
void printHelp();
int getCredentials(Credentials *target);
uint16_t askForInteger(char *prompt);
void upgrade_handle(int socket);
void start_loop(char *hostname, char *port);
void update_welcome(int socket, Message *m);
void add_language(int socket, Message *m);
void *server_thread(void *arg);

#endif /* CLIENT_H */
