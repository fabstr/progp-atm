#include "manage.h"

#define DBPATH "db.sqlite"

char *insert_string = "INSERT INTO accounts (cardnumber, pinhash, balance) "
	"VALUES (?1, ?2, ?3)";

#define STRING_COUNT 20
char *queryStrings[STRING_COUNT] = {
	"Language code: ",
	"Language name: ",
	"Balance command: ",
	"Deposit command: ",
	"Withdraw command: ",
	"Help command: ",
	"Unknown command: ",
	"Balance error: ",
	"Dedposit error: ",
	"Withdraw error: ",
	"Welcome message: ",
	"Balance message: ",
	"Desposit message: ",
	"Withdraw message: ",
	"Help message: ",
	"Enter amount requst: ",
	"Enter one time key request: ",
	"Enter card number request: ",
	"Enter pin request: ",
};

int main(int argc, char **argv)
{
	while (1) {
		char *cmd = readline(">> ");
		if (strcmp(cmd, "quit") == 0) {
			free(cmd);
			break;
		} else if (strcmp(cmd, "add language") == 0) {
			addLanguage();
		} else if (strcmp(cmd, "change welcome") == 0) {
			changeWelcome();
		} else if (strcmp(cmd, "add account") == 0) {
			addAccount();
		} else if (strcmp(cmd, "help") == 0) {
			printHelp();
		}
		free(cmd);
	}

	return EXIT_SUCCESS;
}

void printHelp()
{
	printf("+----------------+\n");
	printf("| Commands:      |\n");
	printf("+----------------+\n");
	printf("| add language   |\n");
	printf("| change welcome |\n");
	printf("| add account    |\n");
	printf("| quit           |\n");
	printf("| help           |\n");
        printf("+----------------+\n");
}

int addLanguage()
{
	/* get the hostname */
	char *host = readline("Hostname: ");

	/* open a socket to the host */
	int socket = connectToServer(host, UPGRPORT);

	/* we're done with host */
	free(host);

	/* variables for ssl */
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	ssl_context ssl;

	/* initialize */
	init_ssl(&ssl, &entropy, &ctr_drbg, &socket, SSL_IS_SERVER);


	/*
	 * send a language_add message and tell the client we are sending
	 * STRING_COUNT network strings
	 */
	Message m = {
		.message_id = language_add,
		.sum = STRING_COUNT,
	};


	mlog("manage.log", "entering addLanguage loop");
	sendMessage(&ssl, &m);

	/*
	 * for each string in queryStrings, ask for it and send the result to
	 * the host
	 */
	int i;
	for (i=0; i<STRING_COUNT; i++) {
		char *str = readline(queryStrings[i]);
		if (str == NULL) {
			fprintf(stderr, "Fatal error: could not read string.\n");
			return 1;
		} else if (sendNetworkString(&ssl, str) != 0) {
			fprintf(stderr, "Could not send network string: %s\n", 
					strerror(errno));
			free(str);
			close(socket);
			return 1;
		}
		free(str);
	}

	ssl_free(&ssl);
	net_close(socket);
	mlog("manage.log", "addLanguage sent");

	return 0;
}

int changeWelcome()
{
	char *msg = readline("New welcome message: ");
	char *lang = readline("Language code: ");
	char *host = readline("Host: ");

	int socket = connectToServer(host, UPGRPORT);

	/* variables for ssl */
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	ssl_context ssl;

	/* initialize */
	init_ssl(&ssl, &entropy, &ctr_drbg, &socket, SSL_IS_SERVER);

	int toReturn = 0;

	mlog("manage.log", "sending welcome update");

	/*
	 * send a welcome_update message and tell the client we are sending
	 * two network strings
	 */
	Message m = {
		.message_id = welcome_update,
		.sum = 2,
	};

	if (sendMessage(&ssl, &m) == -1) {
		mlog("manage.log", "Could not send language_add message: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else if (sendNetworkString(&ssl, lang) == -1) {
		mlog("manage.log", "Could not send network string: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else if (sendNetworkString(&ssl, msg) == -1) {
		mlog("manage.log", "Could not send network string: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else {
		mlog("manage.log", "welcome update sent");
	}

	net_close(socket);
	return toReturn;
}

void addAccount()
{
	mlog("manage.log", "adding account");
	Message m = {
		.message_id = add_account,
		.sum = 0,
	};
	
	/* read the properties */
	mlog("manage.log", "reading properties");
	char *hostname, *pin, *card_number;
	hostname = readline("The server host: ");
	card_number = readline("Card number: ");
	pin = readline("Pin: ");

	/* convert to integers */
	mlog("manage.log", "converting to integers");
	sscanf(pin, "%d", (int *) &m.pin);
	sscanf(card_number, "%d", (int *) &m.card_number);

	/* connect to the server and send the message */
	mlog("manage.log", "sending message");
	int socket = connectToServer(hostname, PORT);

	/* variables for ssl */
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	ssl_context ssl;

	/* initialize */
	init_ssl(&ssl, &entropy, &ctr_drbg, &socket, SSL_IS_SERVER);

	int sent = sendMessage(&ssl, &m);
	mlog("manage.log", "sent %d bytes", sent);
	net_close(socket);

	/* free the strings */
	mlog("manage.log", "freeing memory");
	free(hostname);
	free(pin);
	free(card_number);

	mlog("manage.log", "account added");
}
