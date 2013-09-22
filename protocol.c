#include "protocol.h"

int connectToServer(char *hostname, char *port)
{
	int sock;
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int error;
	char other_ip[INET6_ADDRSTRLEN];

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

	return sock;
}

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress)
{
	if (socketaddress->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) socketaddress)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) socketaddress)->sin6_addr);
}

int getMessage(int connection, Message *target)
{
	NetworkMessage nm;
	int received;
	if ((received = recv(connection, &nm, sizeof(NetworkMessage), 0)) == -1) {
		return -1;
	}

	target->message_id = (Message_ID) nm.message_id;
	target->sum = ntohs(nm.sum);
	target->pin = ntohs(nm.pin);
	target->onetimecode = nm.onetimecode;
	target->card_number = ntohl(nm.card_number);

	return 0;
}

size_t sendMessage(int connection, Message *msg)
{
	uint8_t message_id = (uint8_t) msg->message_id;
	NetworkMessage toSend = {
		.message_id = message_id,
		.sum = htons(msg->sum),
		.pin =  htons(msg->pin),
		.onetimecode =  msg->onetimecode,
		.card_number = htonl(msg->card_number),
	};

	return send(connection, &toSend, sizeof(NetworkMessage), 0);
}

void printMessage(Message *m)
{
	printf("message_id: %d\n", m->message_id);
	printf("sum: %d\n", m->sum);
	printf("pin: %d\n", m->pin);
	printf("onetimecode: %d\n", m->onetimecode);
	printf("card_number: %d\n", m->card_number);
}

int sendNetworkString(int socket, char *string)
{
	uint8_t length = strlen(string);
	if (send(socket, &length, 1, 0) <= 0) {
		return -1;
	} else if (send(socket, string, length, 0) <= 0) {
		return -1;
	}

	return 0;
}

int getNetworkString(int socket, NetworkString *nstr)
{
	mlog("server.log", "getNetworkString");

	/* first receive the length */
	uint8_t length;
	if (recv(socket, &length, 1, 0) == -1) {
		return 1;
	}

	mlog("server.log", "got length = %d", length);

	/* then receive the string */
	nstr->string = (char *) malloc(length + 1);
	size_t received;
	if ((received = recv(socket, nstr->string, length, 0)) == -1) {
		free(nstr->string);
		return 1;
	}

	nstr->string_length = length;
	nstr->string[length] = '\0';

	mlog("server.log", "read %d bytes and got string '%s'", (int) received,
			nstr->string);

	return 0;
}

int start_server(char *port, void(*handle)(int), int *sock)
{
	int new_connection;

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
		if ((*sock = socket(p->ai_family, p->ai_socktype, 
						p->ai_protocol)) == -1) {
			mlog("server.log", "socket: %s", strerror(errno));
			continue;
		}

		if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, 
					sizeof(int)) == -1) {
			perror("setsockopt");
			return EXIT_FAILURE;
		}

		if (bind(*sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(*sock);
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

	if (listen(*sock, BACKLOG) == -1) {
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
		new_connection = accept(*sock,
				(struct sockaddr *) &other_address, &sin_size);

		mlog("server.log", "accepted %d", new_connection);

		if (new_connection == -1) {
			mlog("server.log", "accept: %s", strerror(errno));
			continue;
		}

		inet_ntop(other_address.ss_family, 
				get_ipv4_or_ipv6_addr((struct sockaddr *) 
				&other_address), other_ip, sizeof(other_ip));
		mlog("server.log", "got connection (%d) from %s", new_connection, 
			other_ip);

		if (!fork()) {
			/* child process */
			mlog("server.log", "fork: calling handle");
			close(*sock);
			handle(new_connection);
			close(new_connection);
			return EXIT_SUCCESS;
		}

		close(new_connection);
	}

	close(*sock);
	return EXIT_SUCCESS;
}

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

