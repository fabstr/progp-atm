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

#include "mlog.h"

/* port for normal client/server communication */
#define PORT "4567"

/* port for manage/client upgrade communication */
#define UPGRPORT "4568"

#define BACKLOG 10

#define BUFFSIZE 10

typedef enum {
	/* "normal" messages */
	no, balance, withdraw, deposit,

	/* update messages */
	atm_key, language_add, welcome_update,
} Message_ID;

typedef struct {
	Message_ID message_id;
	uint16_t sum;
	uint16_t pin;
	uint8_t onetimecode;
	uint32_t card_number;
} Message;

typedef struct {
	uint8_t message_id;
	uint16_t sum;
	uint16_t pin;
	uint8_t onetimecode;
	uint32_t card_number;
} NetworkMessage;

typedef struct {
	uint8_t string_length; /**> length of the following string */
	char *string; /**> the string */
} NetworkString;
 
typedef struct {
	uint32_t card_number;
	uint16_t pin;
	uint8_t otp;
} Credentials;

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);
int getMessage(int connection, Message *target);
size_t sendMessage(int connection, Message *msg);
void printMessage(Message *m);
int getCredentials(Credentials *target);
int getCredentialsWithOTP(Credentials *target);
int connectToServer(char *hostname, char *port);
int sendNetworkString(int socket, char *string);
int start_server(char *port, void(*handle)(int));
void sigchld_handler(int s);
int getNetworkString(int socket, NetworkString *nstr);

#endif /* PROTOCOL_H */
