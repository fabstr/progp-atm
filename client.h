#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "mlog.h"
#include "protocol.h"

int connectToServer(char *hostname, char *port);
void start_loop(char *hostname, char *port);

void show_balance(int socket, Credentials *c);
void deposit(int socket, Credentials *c);
void withdraw(int socket, Credentials *c);
void printHelp();
int getCredentials(Credentials *target);
uint16_t askForInteger(char *prompt);

#endif /* CLIENT_H */
