#include "client.h"

int upgrade_socket; 

/* the language code for the current language in the ui */
char *language_code;
bool language_is_changed = false;

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s hostname\n", argv[0]);
		return EXIT_FAILURE;
	}

	bool start_upgrade_server = true;
	if (argc >= 3) {
		if (strcmp(argv[2], "--no-upgrade") == 0) {
			start_upgrade_server = false;
		}
	}

	/* start server for upgrading */
	pthread_t serverthread;
	if (start_upgrade_server == true) {
		pthread_create(&serverthread, NULL, server_thread, NULL);
	} else {
		printf("no upgrade server\n");
	}

	char *hostname = argv[1];
	char *port = PORT;

	/* connect to the database */
	setup_db();

	/* connect to the server and start the main loop */
	int socket = connectToServer(hostname, port);

	/* set default language */
	language_code = (char *) malloc(3);
	language_code[0] = 's';
	language_code[1] = 'v';
	language_code[2] = '\0';
	
	start_loop(socket);

	/* cleanup starts here */
	close(socket);

	mlog("client.log", "cancelling serverthread");
	if (start_upgrade_server == true) {
		pthread_cancel(serverthread);
		pthread_join(serverthread, NULL);
		mlog("client.log", "closing upgrade_socket");
		close(upgrade_socket);
	}


	mlog("client.log", "closing db");
	close_db();
	free(language_code);

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

void start_loop(int socket)
{
	Credentials c;
	bool haveCredentials = false;
	bool looping = true;

	/* our command strings */
	char *str_poweroff = "poweroff";
	char *str_quit = getString(cmd_quit, language_code);
	char *str_balance = getString(cmd_balance, language_code);
	char *str_deposit = getString(cmd_deposit, language_code);
	char *str_withdraw = getString(cmd_withdraw, language_code);
	char *str_unknown_command = getString(error_unknown_command,
			language_code);
	char *str_help = getString(msg_help, language_code);
	char *str_help_cmd = getString(cmd_help, language_code);
	char *str_change_language = "change language";

	
	/* main loop */
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

		/* do something with the command */
		if (strcmp(line, str_poweroff) == 0) {
			looping = false;
		} else if (strcmp(line, str_quit) == 0) {
			haveCredentials = false;
		} else if (strcmp(line, str_balance) == 0) {
			show_balance(socket, &c);
		} else if (strcmp(line, str_deposit) == 0) {
			deposit_money(socket, &c);
		} else if (strcmp(line, str_withdraw) == 0) {
			withdraw_money(socket, &c);
		} else if (strcmp(line, str_help_cmd) == 0) {
			printf("%s\n", str_help);
		} else if (strcmp(line, "") == 0) {
			/* do nothing */
		} else if (strcmp(line, str_change_language) == 0) {
			changeLanguage();
			language_is_changed = true;
		} else {
			printf("%s\n", str_unknown_command);
		}

		if (language_is_changed == true) {
			free(str_quit);
			free(str_balance);
			free(str_deposit);
			free(str_withdraw);
			free(str_unknown_command);
			free(str_help);
			free(str_help_cmd);
			str_quit = getString(cmd_quit, language_code);
			str_balance = getString(cmd_balance, language_code);
			str_deposit = getString(cmd_deposit, language_code);
			str_withdraw = getString(cmd_withdraw, language_code);
			str_unknown_command = getString(error_unknown_command,
					language_code);
			str_help = getString(msg_help, language_code);
			str_help_cmd = getString(cmd_help, language_code);
		}
		free(line);
	}

	Message m = {.message_id = close_connection};
	sendMessage(socket, &m);

	/* free the command strings */
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
		char *pinstring = getpass(str);
		res = sscanf(pinstring, "%hd", &(target->pin));
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
	free(language_code);
	language_is_changed = true;
	language_code = code;
}
