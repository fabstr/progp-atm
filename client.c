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

#include "client.h"
#include "mlog.h"
#include "protocol.h"

int main(int argc, char **argv)
{
	int sock;
	/*int bytes_read;*/
	/*char buff[BUFFSIZE];*/
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int error;
	char other_ip[INET6_ADDRSTRLEN];

	if (argc != 3) {
		fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *hostname = argv[1];
	char *port = argv[2];


	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((error = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
		return EXIT_FAILURE;
	}

	for (p=servinfo; p != NULL; p = p->ai_next) {
		if ((sock = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("client: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect");
		return EXIT_FAILURE;
	}

	inet_ntop(p->ai_family,
			get_ipv4_or_ipv6_addr((struct sockaddr *) p->ai_addr),
			other_ip, sizeof(other_ip));
	printf("client: connecting to %s\n", other_ip);

	freeaddrinfo(servinfo);

	start_loop(sock);
	close(sock);

	return EXIT_SUCCESS;
}

void start_loop(int socket)
{
	printf("Welcome! Write 'help' for help.\n");

	Credentials c;
	getCredentials(&c);

	while (1) {
		char *line = readline(">> ");
		if (!line) {
			break;
		}
		add_history(line);

		if (strcmp(line, "quit") == 0) {
			break;
		} else if (strcmp(line, "exit") == 0) {
			break;
		} else if (strcmp(line, "show balance") == 0) {
			show_balance(socket, &c);
		} else if (strcmp(line, "deposit") == 0) {
			deposit(socket, &c);
		} else if (strcmp(line, "withdraw") == 0) {
			withdraw(socket, &c);
		} else if (strcmp(line, "help") == 0) {
			printHelp();
		} else if (strcmp(line, "'help'") == 0) {
			printHelp();
		} else if (strcmp(line, "") == 0) {
			continue;
		} else {
			printf("Unknown command '%s'. Write 'help' for help.\n",
					line);
		}
		free(line);
	}
}

void printHelp()
{
	printf("COMMAND      DESCRIPTION\n");
	printf("exit         Quit the program. Same as quit.\n");
	printf("quit         Quit the program. Same as exit.\n");
	printf("show balance Show the balance on your account.\n");
	printf("deposit      Deposit money to your account.\n");
	printf("withdraw     Withdraw money from your account.\n");
	printf("help         Show this text.\n");
}

void show_balance(int socket, Credentials *c)
{
	Message m = {
		.message_id = MESSAGE_ID_BALANCE,
		.message_type = MESSAGE_TYPE_ATM_TO_SERVER,
		.atm_id = 0,
		.sum = 0,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = 0
	};

	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id == MESSAGE_ID_NO) {
		printf("Could not get the balance. Please make sure you are using the corrent pin.\n");
	} else {
		printf("Balance: %d\n", answer.sum);
	}
}

void deposit(int socket, Credentials *c)
{
	uint16_t amount = askForInteger("Please enter the amount to deposit (you would be most kind to also insert the money into the machine): ");

	Message m = {
		.message_id = MESSAGE_ID_DEPOSIT,
		.message_type = MESSAGE_TYPE_ATM_TO_SERVER,
		.atm_id = 0,
		.sum = amount,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = 0
	};
	
	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id == MESSAGE_ID_NO) {
		printf("Could not deposit money. Please make sure you are using the corrent pin.\n");
	} else {
		printf("%d was deposited into your account.\n", answer.sum);
	}
}

void withdraw(int socket, Credentials *c)
{
	uint16_t amount = askForInteger("Please enter the amount to withdraw: ");
	uint8_t otp = askForInteger("Please enter your one time key: ");

	Message m = {
		.message_id = MESSAGE_ID_DEPOSIT,
		.message_type = MESSAGE_TYPE_ATM_TO_SERVER,
		.atm_id = 0,
		.sum = amount,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = otp,
	};
	
	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id == MESSAGE_ID_NO) {
		printf("Could not withdraw money.\n");
		printf("Please make sure you are using the corrent pin and ");
		printf("the corrent one time key.\n");
	} else {
		printf("%d was withdrawn from your account.\n", answer.sum);
	}
}

int getCredentials(Credentials *target)
{
	int res = 0;
	while (res == 0) {
		char *cardstring = readline("Please enter your card number: ");
		res = sscanf(cardstring, "%d", &(target->card_number));
		free(cardstring);
	}

	res = 0;
	while (res == 0) {
		char *pinstring = readline("Please enter your card number: ");
		res = sscanf(pinstring, "%hd", &(target->pin));
		free(pinstring);
	}

	return 0;
}

uint16_t askForInteger(char *prompt)
{
	int res = 0;
	uint16_t result;
	while (res == 0) {
		char *integerstring = readline(prompt);
		res = sscanf(integerstring, "%d", (int *) &result);
		free(integerstring);
	}

	return result;
}
