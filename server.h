#ifndef SERVER_H
#define SERVER_H

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

#include "mlog.h"
#include "protocol.h"
#include "serverdb.h"

void sigchld_handler(int s);

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);

void handle_connection(int socket);

#endif /* SERVER_H */
