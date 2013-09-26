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
	bool looping = true;

	char *str_poweroff = "poweroff";
	char *str_quit = getString(cmd_quit, language_code);
	char *str_balance = getString(cmd_balance, language_code);
	char *str_deposit = getString(cmd_deposit, language_code);
	char *str_withdraw = getString(cmd_withdraw, language_code);
	char *str_unknown_command = getString(error_unknown_command, language_code);
	char *str_help = getString(msg_help, language_code);
	char *str_help_cmd = getString(cmd_help, language_code);
	char *str_change_language = "change language";

	while (looping == true) {
		if (haveCredentials == false) {
			char *str = getString(msg_welcome, language_code);
			printf("%s\n", str);
			getCredentials(&c);
			haveCredentials = true;
			free(str);
		}

		char *line = readline(">> ");
		add_history(line);

		if (strcmp(line, str_poweroff) == 0) {
			looping = false;
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
		} else if (strcmp(line, str_help_cmd) == 0) {
			printf("%s\n", str_help);
		} else if (strcmp(line, "") == 0) {
			/* do nothing */
		} else if (strcmp(line, str_change_language) == 0) {
			changeLanguage();
		} else {
			printf("%s\n", str_unknown_command);
		}
		free(line);
	}

	free(str_quit);
	free(str_balance);
	free(str_deposit);
	free(str_withdraw);
	free(str_unknown_command);
	free(str_help);
	free(str_help_cmd);
}

void show_balance(int socket, Credentials *c)
{
	Message *m = (Message *) malloc(sizeof(Message));
	m->message_id = balance;
	m->sum = 0;
	m->pin = c->pin;
	m->card_number = c->card_number;
	m->onetimecode = 0;

	Message answer;
	sendMessage(socket, m);
	getMessage(socket, &answer);

	if (answer.message_id != balance) {
		char *str = getString(error_balance, language_code);
		printf("%s\n", str);
		free(str);
	} else {
		char *str = getString(msg_balance, language_code);
		printf("%s", str);
		printf("%d\n", answer.sum);
		free(str);
	}

	free(m);
}

void deposit_money(int socket, Credentials *c)
{
	char *str1 = getString(rqst_enter_amount, language_code);
	uint16_t amount = askForInteger(str1);
	free(str1);

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
		char *str2 = getString(error_deposit, language_code);
		printf("%s\n", str2);
		free(str2);
	} else {
		char *str3 = getString(msg_deposit, language_code);
		printf("%s %d\n", str3, m.sum);
		free(str3);
	}
}

void withdraw_money(int socket, Credentials *c)
{
	char *str1 = getString(rqst_enter_amount, language_code);
	uint16_t amount = askForInteger(str1);
	free(str1);

	char *str2 = getString(rqst_enter_otp, language_code);
	uint8_t otp = askForInteger(str2);
	free(str2);

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
		char *str3 = getString(error_withdraw, language_code);
		printf("%s\n", str3);
		free(str3);
	} else {
		char *str4 = getString(msg_withdraw, language_code);
		printf("%s\n", str4);
		printf("%d\n", answer.sum);
		free(str4);
	}
}

int getCredentials(Credentials *target)
{
	int res = 0;
	while (res == 0) {
		char *str = getString(rqst_card_number, language_code);
		char *cardstring = readline(str);
		res = sscanf(cardstring, "%d", &(target->card_number));
		free(cardstring);
		free(str);
	}

	res = 0;
	while (res == 0) {
		char *str = getString(rqst_pin, language_code);
		char *pinstring = readline(str);
		res = sscanf(pinstring, "%hd", &(target->pin));
		free(pinstring);
		free(str);
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
