#include "networking.h"

int connectToServer(char *hostname, char *port)
{
	int sock, ret;
	int p;
	sscanf(port, "%d", &p);
	if ((ret = net_connect(&sock, hostname, p)) != 0) {
		mlog("client.log", "failed to connect");
		return EXIT_FAILURE;
	}

	return sock;
}

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress)
{
	if (socketaddress->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) socketaddress)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) socketaddress)->sin6_addr);
}

int getMessage(ssl_context *ssl, Message *target)
{
	NetworkMessage nm;
	int read = ssl_read(ssl, (unsigned char *) &nm, sizeof(NetworkMessage));
	if (read <= 0) {
		return -1;
	}

	target->message_id = (Message_ID) nm.message_id;
	target->sum = ntohs(nm.sum);
	target->pin = ntohs(nm.pin);
	target->onetimecode = nm.onetimecode;
	target->card_number = ntohs(nm.card_number);
	return 0;
}

size_t sendMessage(ssl_context *ssl, Message *msg)
{
	NetworkMessage toSend;
	memset(&toSend, 0, sizeof(NetworkMessage));

	uint8_t message_id = (uint8_t) msg->message_id;

	toSend.message_id = message_id;
	toSend.sum = htons(msg->sum);
	toSend.pin =  htons(msg->pin);
	toSend.onetimecode =  msg->onetimecode;
	toSend.card_number = htons(msg->card_number);

	int toReturn = ssl_write(ssl, (unsigned char *) &toSend, 
			sizeof(NetworkMessage));

	return toReturn;
}

void printMessage(Message *m)
{
	printf("message_id: %d\n", m->message_id);
	printf("sum: %d\n", m->sum);
	printf("pin: %d\n", m->pin);
	printf("onetimecode: %d\n", m->onetimecode);
	printf("card_number: %d\n", m->card_number);
}

int sendNetworkString(ssl_context *ssl, char *string)
{
	uint8_t length = strlen(string);
	if (ssl_write(ssl, &length, 1) <= 0) {
		return -1;
	} else if (ssl_write(ssl, (unsigned char *) string, length) <= 0) {
		return -1;
	}

	return 0;
}

int getNetworkString(ssl_context *ssl, NetworkString *nstr)
{
	mlog("server.log", "getNetworkString");

	/* first receive the length */
	uint8_t length;
	if (ssl_read(ssl, &length, 1) == -1) {
		return 1;
	}

	mlog("server.log", "got length = %d", length);

	/* then receive the string */
	nstr->string = (char *) malloc(length + 1);
	size_t received;
	if ((received = ssl_read(ssl, (unsigned char *) nstr->string, length)) 
			== -1) {
		free(nstr->string);
		return 1;
	}

	nstr->string_length = length;
	nstr->string[length] = '\0';

	mlog("server.log", "read %d bytes and got string '%s'", (int) received,
			nstr->string);

	return 0;
}

int start_server(char *port, void(*handle)(ssl_context*), int *sock)
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
		mlog("server.log", "server: failed to bind\n");
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
				&other_address),
				other_ip,
				sizeof(other_ip));
		mlog("server.log", "got connection (%d) from %s",
				new_connection, other_ip);

		if (!fork()) {
			/* child process */
			mlog("server.log", "fork: processing");
			close(*sock);
			process_client(new_connection, handle);
			return EXIT_SUCCESS;
		}

		close(new_connection);
	}

	close(*sock);
	return EXIT_SUCCESS;
}

void ssl_debug(void *ctx, int level, const char *str)
{
	((void)level);
	fprintf((FILE *) ctx, "%s", str);
	fflush((FILE *) ctx);
}

int init_ssl(ssl_context *ssl, entropy_context *entropy, 
		ctr_drbg_context *ctr_drbg, int *sockfd, int endpoint)
{
	/* initialize ssl */
	init_ctr(entropy, ctr_drbg);
	memset(ssl, 0, sizeof(ssl_context));
	int ret;
	if ((ret = ssl_init(ssl)) != 0) {
		mlog("server.log", "fatal error: ssl_init returned %d", ret);
		return ret;
	}
	ssl_set_endpoint(ssl, endpoint);
	ssl_set_authmode(ssl, SSL_VERIFY_NONE);
	ssl_set_rng(ssl, ctr_drbg_random, ctr_drbg);
	ssl_set_dbg(ssl, ssl_debug, stderr);
	ssl_set_bio(ssl, net_recv, sockfd, net_send, sockfd);
	ssl_set_ciphersuites(ssl, ssl_list_ciphersuites());

	return 0;
}

void process_client(int sockfd, void(*handle)(ssl_context*))
{
	/* variables for ssl */
	entropy_context entropy;
	ctr_drbg_context ctr_drbg;
	ssl_context ssl;

	/* initialize */
	init_ssl(&ssl, &entropy, &ctr_drbg, &sockfd, SSL_IS_SERVER);

	/* call the handle */
	handle(&ssl);

	/* free ssl and close the socket */
	ssl_free(&ssl);
	close(sockfd);
}

int init_ctr(entropy_context *entropy, ctr_drbg_context *ctr_drbg)
{
	entropy_init(entropy);
	int ret = ctr_drbg_init(ctr_drbg, entropy_func, entropy, NULL, 0);
	if (ret != 0) {
		mlog("ssl.log", "ctr_drbg_init returned %d", ret);
		return ret;
	}

	return 0;
}

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

