#include "manage.h"

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
			break;
		} else if (strcmp(cmd, "add language") == 0) {
			addLanguage();
		} else if (strcmp(cmd, "change welcome") == 0) {
			changeWelcome();
		} else if ((strcmp(cmd, "help") == 0) || 
				strcmp(cmd, "'help'") == 0) {
			printHelp();
		}
		free(cmd);
	}

	return EXIT_SUCCESS;
}

void printHelp()
{
	printf("There is no help.\n");
}

int addLanguage()
{
	/* get the hostname */
	char *host = readline("Hostname: ");

	/* open a socket to the host */
	int socket = connectToServer(host, UPGRPORT);

	/* we're done with host */
	free(host);

	/*
	 * send a language_add message and tell the client we are sending
	 * STRING_COUNT network strings
	 */
	Message m = {
		.message_id = language_add,
		.sum = STRING_COUNT,
	};


	mlog("manage.log", "entering addLanguage loop");
	sendMessage(socket, &m);

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
		} else if (sendNetworkString(socket, str) != 0) {
			fprintf(stderr, "Could not send network string: %s\n", 
					strerror(errno));
			free(str);
			close(socket);
			return 1;
		}
		free(str);
	}

	close(socket);
	mlog("manage.log", "addLanguage sent");

	return 0;
}

int changeWelcome()
{
	char *msg = readline("New welcome message: ");
	char *lang = readline("Language code: ");
	char *host = readline("Host: ");

	int socket = connectToServer(host, UPGRPORT);
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

	if (sendMessage(socket, &m) == -1) {
		mlog("manage.log", "Could not send language_add message: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else if (sendNetworkString(socket, lang) == -1) {
		mlog("manage.log", "Could not send network string: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else if (sendNetworkString(socket, msg) == -1) {
		mlog("manage.log", "Could not send network string: %s\n", 
				strerror(errno));
		toReturn = 1;
	} else {
		mlog("manage.log", "welcome update sent");
	}

	close(socket);
	return toReturn;
}
