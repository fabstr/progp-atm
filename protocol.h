#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PORT "4567"
#define BUFFSIZE 10

#define ATM_ID_MASK 0xF0
#define MESSAGE_ID_MASK 0x0C
#define MESSAGE_TYPE_MASK 0x03

#define ATM_ID_SHIFT 4
#define MESSAGE_ID_SHIFT 2
#define MESSAGE_TYPE_SHIFT 0

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

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);
int getMessage(int connection, Message *target);
size_t sendMessage(int connection, Message *msg);
void printMessage(Message *m);

#endif /* PROTOCOL_H */
