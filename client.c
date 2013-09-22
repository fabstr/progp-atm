#include "client.h"

int upgrade_socket; 

/* the language code for the current language in the ui */
char *language_code = "sv";

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

	setup_db();

	start_loop(hostname, port);

	mlog("client.log", "cancelling serverthread");
	pthread_cancel(serverthread);
	pthread_join(serverthread, NULL);
	mlog("client.log", "closing upgrade_socket");
	close(upgrade_socket);
	close_db();

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
	mlog("client.log", "got connection (%d)", socket);
	extern pthread_mutex_t dbmutex;

	Message m;
	getMessage(socket, &m);
	switch (m.message_id) {
	case welcome_update:
		mlog("client.log", "got upgrade welcome");
		pthread_mutex_lock(&dbmutex);
		update_welcome(socket, &m);
		pthread_mutex_unlock(&dbmutex);
		break;
	case language_add:
		mlog("client.log", "got add language");
		pthread_mutex_lock(&dbmutex);
		add_language(socket, &m);
		pthread_mutex_unlock(&dbmutex);
		break;
	default:
		mlog("client.log", "invalid message id.");
		break;
	}
}

void update_welcome(int socket, Message *m)
{
	int nNetworkStrings = m->sum;
	int nStringsGotten = 0;
	int i;
	NetworkString strings[nNetworkStrings];

	mlog("client.log", "nNetworkStrings = %d", nNetworkStrings);

	for (i=0; i<nNetworkStrings; i++) {
		if (getNetworkString(socket, &(strings[i])) != 0) {
			mlog("client.log", "Could not get network string: %s\n",
					strerror(errno));
		} else {
			nStringsGotten++;
		}
	}

	mlog("client.log", "got %d strings", nStringsGotten);

	/* i starts at zero */
	if (nStringsGotten != 2) {
		mlog("client.log", "upgrade protocol error: should receive "
				"2 network strings when updating welcome, got "
				"%d.", i);
		return;
	} else {
		char *lang_code = strings[0].string;
		char *newmsg = strings[1].string;
		if (update_welcome_db(lang_code, newmsg) != 0) {
			mlog("client.log", "could not update database.");
		} else {
			mlog("client.log", "updated welcome");
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

	char *str_poweroff = "poweroff";
	char *str_quit = getString(cmd_quit, language_code);
	char *str_balance = getString(cmd_balance, language_code);
	char *str_deposit = getString(cmd_deposit, language_code);
	char *str_withdraw = getString(cmd_withdraw, language_code);
	char *str_help = getString(cmd_help, language_code);
	char *str_change_language = "change language";

	while (1) {
		if (haveCredentials == false) {
			printf("%s\n", getString(msg_welcome, language_code));
			getCredentials(&c);
			haveCredentials = true;
		}

		char *line = readline(">> ");
		if (!line) {
			break;
		}
		add_history(line);

		if (strcmp(line, str_poweroff) == 0) {
			break;
		} else if (strcmp(line, str_quit) == 0) {
			haveCredentials = false;
		} else if (strcmp(line, str_balance) == 0) {
			int socket = connectToServer(hostname, port);
			show_balance(socket, &c);
			close(socket);
		} else if (strcmp(line, str_deposit) == 0) {
			int socket = connectToServer(hostname, port);
			deposit_money(socket, &c);
			close(socket);
		} else if (strcmp(line, str_withdraw) == 0) {
			int socket = connectToServer(hostname, port);
			withdraw_money(socket, &c);
			close(socket);
		} else if (strcmp(line, str_help) == 0) {
			printf("%s\n", getString(cmd_help, language_code));
		} else if (strcmp(line, "") == 0) {
			continue;
		} else if (strcmp(line, str_change_language) == 0) {
			changeLanguage();
		} else {
			printf("%s\n", getString(error_unknown_command, language_code));
		}
		free(line);
	}
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
		printf("%s\n", getString(error_balance, language_code));
	} else {
		printf("%s", getString(msg_balance, language_code));
		printf("%d\n", answer.sum);
	}
}

void deposit_money(int socket, Credentials *c)
{
	uint16_t amount = askForInteger(getString(rqst_enter_amount, language_code));

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
		printf("%s\n", getString(error_deposit, language_code));
	} else {
		printf("%s %d\n", getString(msg_deposit, language_code), m.sum);
	}
}

void withdraw_money(int socket, Credentials *c)
{
	uint16_t amount = askForInteger(getString(rqst_enter_amount, language_code));
	uint8_t otp = askForInteger(getString(rqst_enter_otp, language_code));

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
		printf("%s\n", getString(error_withdraw, language_code));
	} else {
		printf("%s", getString(msg_withdraw, language_code));
		printf("%d\n", answer.sum);
	}
}

int getCredentials(Credentials *target)
{
	int res = 0;
	while (res == 0) {
		char *cardstring = readline(getString(rqst_card_number, language_code));
		res = sscanf(cardstring, "%d", &(target->card_number));
		free(cardstring);
	}

	res = 0;
	while (res == 0) {
		char *pinstring = readline(getString(rqst_pin, language_code));
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

void changeLanguage()
{
	printAvaliableLanguages();

	char *code = readline("Enter the code to use: ");

	while (correctLangCode(code) == false) {
		printf("Invalid language code.");
		free(code);
		code = readline("Enter the code to use: ");
	}

	setLanguage(code);
}



void setLanguage(char *code)
{
	language_code = code;
}
