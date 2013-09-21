#ifndef CLIENT_H
#define CLIENT_H

#include "protocol.h"

void start_loop(int socket);

void show_balance(int socket, Credentials *c);
void deposit(int socket, Credentials *c);
void withdraw(int socket, Credentials *c);
void printHelp();
int getCredentials(Credentials *target);
uint16_t askForInteger(char *prompt);

#endif /* CLIENT_H */
