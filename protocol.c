#include "protocol.h"

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress)
{
	if (socketaddress->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) socketaddress)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) socketaddress)->sin6_addr);
}

int getMessage(int connection, Message *target)
{
	NetworkMessage nm;
	int received;
	if ((received = recv(connection, &nm, sizeof(NetworkMessage), 0)) == -1) {
		return -1;
	}

	target->atm_id = (nm.byte0 & ATM_ID_MASK) >> ATM_ID_SHIFT;
	target->message_id = (nm.byte0 & MESSAGE_ID_MASK) >> MESSAGE_ID_SHIFT;
	target->message_type = (nm.byte0 & MESSAGE_TYPE_MASK) >> MESSAGE_TYPE_SHIFT;
	target->sum = ntohs(nm.sum);
	target->pin = ntohs(nm.pin);
	target->onetimecode = nm.onetimecode;
	target->card_number = ntohl(nm.card_number);

	return 0;
}

size_t sendMessage(int connection, Message *msg)
{
	uint8_t byte0 = 0;
	byte0 += msg->atm_id << ATM_ID_SHIFT;
	byte0 += msg->message_id << MESSAGE_ID_SHIFT;
	byte0 += msg->message_type << MESSAGE_TYPE_SHIFT;

	NetworkMessage toSend = {
		.byte0 = byte0,
		.sum = htons(msg->sum),
		.pin =  htons(msg->pin),
		.onetimecode =  msg->onetimecode,
		.card_number = htonl(msg->card_number),
	};

	return send(connection, &toSend, sizeof(NetworkMessage), 0);
}

void printMessage(Message *m)
{
	printf("atm_id: %d\n", m->atm_id);
	printf("message_id: %d\n", m->message_id);
	printf("message_type: %d\n", m->message_type);
	printf("sum: %d\n", m->sum);
	printf("pin: %d\n", m->pin);
	printf("onetimecode: %d\n", m->onetimecode);
	printf("card_number: %d\n", m->card_number);
}
