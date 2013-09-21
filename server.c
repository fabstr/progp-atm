#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "server.h"
#include "mlog.h"
#include "protocol.h"
#include "serverdb.h"

#define BACKLOG 10

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

/**
 * Handle an upgrade message.
 * @param socket The connection to the atm
 * @param m The message that was sent
 */
void handle_upgrade(int socket, Message *m)
{
	switch (m->message_id) {
	default:
		break;
	}
}

/**
 * Handle a normal atm to server message.
 * @param socket The connection to the atm
 * @param m The message that was sent
 */
void handle_normal(int socket, Message *m)
{
	uint16_t balance = 0;
	Message no;
	memset(&no, 0, sizeof(Message));
	no.message_type = MESSAGE_TYPE_SERVER_TO_ATM;

	switch (m->message_id) {
	case MESSAGE_ID_NO:
		/* do nothing */
		mlog("server.log", "%d was no", socket);
		return;
		break;
	case MESSAGE_ID_BALANCE:
		mlog("server.log", "%d was balance", socket);
		if (getBalance(m, &balance) != 0) {
			mlog("server.log", "Could not get balance.");
			*m = no;
		} else {
			mlog("server.log", "got balance %d", balance);
			m->sum = balance;
		}
		break;
	case MESSAGE_ID_WITHDRAWAL:
		mlog("server.log", "%d was withdrawal", socket);
		if (getBalance(m, &balance) != 0) {
			fprintf(stderr, "Could not get balance.\n");
			*m = no;
		}
		if (m->sum > balance) {
			/* we can't overdraft */
			m->message_id = MESSAGE_ID_NO;
		} else {
			m->sum = balance - m->sum;
			update(m);
		}
		break;
	case MESSAGE_ID_DEPOSIT:
		mlog("server.log", "%d was deposit", socket);
		if (getBalance(m, &balance) != 0) {
			fprintf(stderr, "Could not get balance.\n");
			*m = no;
		} else {
			m->sum += balance;
			update(m);
			getBalance(m, &balance);
			mlog("server.log", "balance is now %d for card # %d", 
					balance, m->card_number);
		}
		break;
	}

	sendMessage(socket, m);
}

void handle_connection(int socket)
{
	mlog("server.log", "handling connection %d", socket);
	Message m;
	getMessage(socket, &m);

	switch (m.message_type) {
	case MESSAGE_TYPE_ATM_TO_SERVER:
		mlog("server.log", "normal message %d", socket);
		handle_normal(socket, &m);
		break;
	case MESSAGE_TYPE_UPGRADE_ATM_TO_SERVER:
		mlog("server.log", "upgrade message %d", socket);
		handle_upgrade(socket, &m);
		break;
	default:
		memset(&m, 0, sizeof(Message));
		m.message_type = MESSAGE_TYPE_SERVER_TO_ATM;
		sendMessage(socket, &m);
		return;
	}

	return;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (setup_db() != 0) {
		fprintf(stderr, "Fatal error: could not start the database.\n");
		return EXIT_FAILURE;
	}

	char *port = argv[1];

	int sock, new_connection;

	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	
	struct sockaddr_storage other_address;
	socklen_t sin_size;
	struct sigaction sigact;
	int yes = 1;
	char other_ip[INET6_ADDRSTRLEN];
	int error;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((error = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		perror("getaddrinfo");
		return EXIT_FAILURE;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock = socket(p->ai_family, p->ai_socktype, 
						p->ai_protocol)) == -1) {
			mlog("server.log", "socket: %s", strerror(errno));
			continue;
		}

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, 
					sizeof(int)) == -1) {
			perror("setsockopt");
			return EXIT_FAILURE;
		}

		if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			mlog("server.log", "bind: %s", strerror(errno));
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return EXIT_FAILURE;
	}

	freeaddrinfo(servinfo);

	if (listen(sock, BACKLOG) == -1) {
		mlog("server.log", "listen: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	sigact.sa_handler = sigchld_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigact, NULL) == -1) {
		mlog("server.log", "sigaction: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	mlog("server.log", "waiting for connections");

	while (true) {
		sin_size = sizeof(other_address);
		new_connection = accept(sock,
				(struct sockaddr *) &other_address, &sin_size);
		if (new_connection == -1) {
			mlog("server.log", "accept: %s", strerror(errno));
			continue;
		}

		inet_ntop(other_address.ss_family, 
				get_ipv4_or_ipv6_addr((struct sockaddr *) 
				&other_address), other_ip, sizeof(other_ip));
		mlog("server.log", "got connection (%d) from %s", new_connection, other_ip);

		if (!fork()) {
			/* child process */
			close(sock);
			handle_connection(new_connection);
			close(new_connection);
			return EXIT_SUCCESS;
		}

		close(new_connection);
	}

	return EXIT_SUCCESS;
}
