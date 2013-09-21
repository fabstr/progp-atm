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

#define BACKLOG 10

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

void handle_connection(int socket)
{
	Message m = {
		.atm_id = 3,
		.message_id = 2,
		.message_type = 1,
		.sum = 1234,
		.pin = 4321,
		.onetimecode = 97,
		.card_number = 12345678
	};

	printf("server: sending: \n");
	printMessage(&m);
	sendMessage(socket, &m);
	return;
}

int main(int argc, char **argv)
{
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

	if ((error = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		perror("getaddrinfo");
		return EXIT_FAILURE;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sock = socket(p->ai_family, p->ai_socktype, 
						p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, 
					sizeof(int)) == -1) {
			perror("setsockopt");
			return EXIT_FAILURE;
		}

		if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("server: bind");
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
		perror("listen");
		return EXIT_FAILURE;
	}

	sigact.sa_handler = sigchld_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sigact, NULL) == -1) {
		perror("sigaction");
		return EXIT_FAILURE;
	}

	printf("server: waiting for connections\n");

	while (true) {
		sin_size = sizeof(other_address);
		new_connection = accept(sock,
				(struct sockaddr *) &other_address, &sin_size);
		if (new_connection == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(other_address.ss_family, 
				get_ipv4_or_ipv6_addr((struct sockaddr *) 
				&other_address), other_ip, sizeof(other_ip));
		printf("server: got connection from %s\n", other_ip);

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

