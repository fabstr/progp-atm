#include "server.h"

#define BACKLOG 10

void handle_upgrade(int socket, Message *m)
{
	/* don't do anything */
}

void handle_normal(int socket, Message *m)
{
	uint16_t account_balance = 0;
	Message nomsg = {};
	/*memset(&nomsg, 0, sizeof(Message));*/
	nomsg.message_id = no;

	uint16_t withdrawn = 0;
	uint16_t deposited = 0;

	switch (m->message_id) {
	case no:
		/* do nothing */
		mlog("server.log", "%d was no", socket);
		return;
		break;
	case balance:
		mlog("server.log", "%d was balance", socket);
		if (getBalance(m, &account_balance) != 0) {
			mlog("server.log", "Could not get balance.");
			*m = nomsg;
		} else {
			mlog("server.log", "got balance %d", account_balance);
			m->sum = account_balance;
		}
		break;
	case withdraw:
		mlog("server.log", "%d was withdrawal", socket);
		if (getBalance(m, &account_balance) != 0) {
			fprintf(stderr, "Could not get balance.\n");
			*m = nomsg;
		}
		if (m->sum > account_balance) {
			/* we can't overdraft */
			m->message_id = no;
		} else {
			uint8_t onetimekey = m->onetimecode;
			getNextOnetimekey(m);
			if (onetimekey != m->onetimecode) {
				mlog("server.log", "wrong otk");
				*m = nomsg;
				break;
			}

			withdrawn = m->sum;
			m->sum = account_balance - m->sum;
			update(m);
			updateOnetimekey(m);
			m->sum = withdrawn;
		}
		break;
	case deposit:
		mlog("server.log", "%d was deposit", socket);
		if (getBalance(m, &account_balance) != 0) {
			fprintf(stderr, "Could not get balance.\n");
			*m = nomsg;
		} else {
			deposited = m->sum;
			m->sum += account_balance;
			update(m);
			getBalance(m, &account_balance);
			mlog("server.log", "balance is now %d for card # %d", 
					balance, m->card_number);
			m->sum = deposited;
		}
		break;
	default:
		*m = nomsg;
		break;
	}

	sendMessage(socket, m);
}

void handle_connection(int socket)
{
	mlog("server.log", "handling connection %d", socket);
	Message *m = (Message *) malloc(sizeof(Message));
	memset(m, 0, sizeof(Message));

	while (m->message_id != close_connection) {
		getMessage(socket, m);

		switch (m->message_id) {
		case balance:
		case withdraw:
		case deposit:
			mlog("server.log", "normal message %d", socket);
			handle_normal(socket, m);
			break;
		case atm_key:
		case language_add:
		case welcome_update:
			mlog("server.log", "upgrade message %d", socket);
			handle_upgrade(socket, m);
			break;
		case close_connection :
			break;
		default:
			m->message_id = no;
			sendMessage(socket, m);
			break;
		}
	}

	mlog("server.log", "closing connection");

	free(m);
	mlog("server.log", "closing db");
	close_db();
	return;
}

int main(int argc, char **argv)
{
	if (setup_db() != 0) {
		fprintf(stderr, "Fatal error: could not start the database.\n");
		return EXIT_FAILURE;
	}

	char *port = PORT;
	int socket;
	return start_server(port, handle_connection, &socket);
}
