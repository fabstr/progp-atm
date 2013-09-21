#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <netinet/in.h>

#define PORT "4567"
#define BUFFSIZE 10

#define ATM_ID_MASK 0xF0
#define MESSAGE_ID_MASK 0x0C
#define MESSAGE_TYPE_MASK 0x03

#define ATM_ID_SHIFT 4
#define MESSAGE_ID_SHIFT 2
#define MESSAGE_TYPE_SHIFT 0

#define MESSAGE_TYPE_ATM_TO_SERVER 0
#define MESSAGE_TYPE_SERVER_TO_ATM 1
#define MESSAGE_TYPE_UPGRADE_SERVER_TO_ATM 2
#define MESSAGE_TYPE_UPGRADE_ATM_TO_SERVER 3

#define MESSAGE_ID_NO 0
#define MESSAGE_ID_BALANCE 1
#define MESSAGE_ID_WITHDRAWAL 2
#define MESSAGE_ID_DEPOSIT 3

typedef struct {
	uint8_t atm_id;
	uint8_t message_id;
	uint8_t message_type;
	uint16_t sum;
	uint16_t pin;
	uint8_t onetimecode;
	uint32_t card_number;
} Message;

typedef struct {
	uint8_t byte0;
	uint16_t sum;
	uint16_t pin;
	uint8_t onetimecode;
	uint32_t card_number;
} NetworkMessage;

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

#endif /* PROTOCOL_H */
