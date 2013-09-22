#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "protocol.h"
#include "serverdb.h"
#include "mlog.h"

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);

void handle_connection(int socket);

#endif /* SERVER_H */
