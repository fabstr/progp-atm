#ifndef SERVER_H
#define SERVER_H

void sigchld_handler(int s);

void *get_ipv4_or_ipv6_addr(struct sockaddr *socketaddress);

void handle_connection(int socket);

#endif /* SERVER_H */
