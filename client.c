#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "mlog.h"
#include "protocol.h"

int main(int argc, char **argv)
{
	int sock;
	/*int bytes_read;*/
	/*char buff[BUFFSIZE];*/
	struct addrinfo hints;
	struct addrinfo *servinfo, *p;
	int error;
	char other_ip[INET6_ADDRSTRLEN];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s hostname\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *hostname = argv[1];


	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((error = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
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

	Message m;
	getMessage(sock, &m);
	printf("client: received\n");
	printMessage(&m);

	close(sock);

	return EXIT_SUCCESS;
}
