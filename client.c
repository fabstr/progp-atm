#include "client.h"

int upgrade_socket; 

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s hostname\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* start server for upgrading */
	pthread_t serverthread;
	pthread_create(&serverthread, NULL, server_thread, NULL);

	char *hostname = argv[1];
	char *port = PORT;
	start_loop(hostname, port);

	mlog("client.log", "cancelling serverthread");
	pthread_cancel(serverthread);
	pthread_join(serverthread, NULL);
	mlog("client.log", "closing upgrade_socket");
	close(upgrade_socket);

	return EXIT_SUCCESS;
}

void *server_thread(void *arg)
{
	mlog("client.log", "starting upgrade server at %s", UPGRPORT);
	start_server(UPGRPORT, upgrade_handle, &upgrade_socket);
	return NULL;
}

void upgrade_handle(int socket)
{
	Message m;
	getMessage(socket, &m);
	switch (m.message_id) {
	case welcome_update:
		update_welcome(socket, &m);
		break;
	case language_add:
		add_language(socket, &m);
		break;
	default:
		mlog("client.log", "invalid message id.");
		break;
	}
}

void update_welcome(int socket, Message *m)
{
	int nNetworkStrings = m->sum;
	int i;
	NetworkString strings[nNetworkStrings];

	for (i=0; i<nNetworkStrings; i++) {
		if (getNetworkString(socket, &(strings[i])) != 0) {
			mlog("client.log", "Could not get network string: %s\n",
					strerror(errno));
		}
	}

	if (i != 2) {
		mlog("client.log", "upgrade protocol error: should receive "
				"2 network strings when updating welcome, got "
				"%d.", i);
		return;
	} else {
		char *lang_code = strings[0].string;
		char *newmsg = strings[1].string;
		if (update_welcome_db(lang_code, newmsg) != 0) {
			mlog("client.log", "could not update database.");
		}
	}

	for (i=0; i<nNetworkStrings; i++) {
		free(strings[i].string);
	}
}

void add_language(int socket, Message *m)
{
	int nNetworkStrings = m->sum;
	int i;
	NetworkString strings[nNetworkStrings];

	for (i=0; i<nNetworkStrings; i++) {
		if (getNetworkString(socket, &(strings[i])) != 0) {
			mlog("client.log", "Could not get network string: %s\n",
					strerror(errno));
		}
	}

	char *lang_strings[nNetworkStrings];
	for (i=0; i<nNetworkStrings; i++) {
		lang_strings[i] = strings[i].string;
	}

	create_language(lang_strings, nNetworkStrings);

	for (i=0; i<nNetworkStrings; i++) {
		free(strings[i].string);
	}
}

void start_loop(char *hostname, char *port)
{
	Credentials c;
	bool haveCredentials = false;

	while (1) {
		if (haveCredentials == false) {
			printf("Välkommen till räksmörgåsen! Write 'help' "
					"for help.\n");
			getCredentials(&c);
			haveCredentials = true;
		}

		char *line = readline(">> ");
		if (!line) {
			break;
		}
		add_history(line);

		if (strcmp(line, "poweroff") == 0) {
			break;
		} else if (strcmp(line, "quit") == 0) {
			haveCredentials = false;
		} else if (strcmp(line, "show balance") == 0) {
			int socket = connectToServer(hostname, port);
			show_balance(socket, &c);
			close(socket);
		} else if (strcmp(line, "deposit") == 0) {
			int socket = connectToServer(hostname, port);
			deposit_money(socket, &c);
			close(socket);
		} else if (strcmp(line, "withdraw") == 0) {
			int socket = connectToServer(hostname, port);
			withdraw_money(socket, &c);
			close(socket);
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
	printf("quit         Quit the program. Same as exit.\n");
	printf("show balance Show the balance on your account.\n");
	printf("deposit      Deposit money to your account.\n");
	printf("withdraw     Withdraw money from your account.\n");
	printf("help         Show this text.\n");
}

void show_balance(int socket, Credentials *c)
{
	Message m = {
		.message_id = balance,
		.sum = 0,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = 0
	};

	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id != balance) {
		printf("Could not get the balance. Please make sure you are using the corrent pin.\n");
	} else {
		printf("Balance: %d\n", answer.sum);
	}
}

void deposit_money(int socket, Credentials *c)
{
	uint16_t amount = askForInteger("Please enter the amount to deposit (you would be most kind to also insert the money into the machine): ");

	Message m = {
		.message_id = deposit,
		.sum = amount,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = 0
	};
	
	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id != deposit) {
		printf("Could not deposit money. Please make sure you are using the corrent pin.\n");
	} else {
		printf("%d was deposited into your account.\n", answer.sum);
	}
}

void withdraw_money(int socket, Credentials *c)
{
	uint16_t amount = askForInteger("Please enter the amount to withdraw: ");
	uint8_t otp = askForInteger("Please enter your one time key: ");

	Message m = {
		.message_id = withdraw,
		.sum = amount,
		.pin = c->pin,
		.card_number = c->card_number,
		.onetimecode = otp,
	};
	
	Message answer;
	sendMessage(socket, &m);
	getMessage(socket, &answer);

	if (answer.message_id != withdraw) {
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
		char *pinstring = readline("Please enter your personal "
				"identification number: ");
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
