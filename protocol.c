#include "protocol.h"

int connectToServer(char *hostname, char *port, SSLConnection *con)
{
	/* first create some objects */
	BIO *bio;
	SSL *ssl;
	SSL_CTX *ctx;

	/* then set up the library and the ssl context */
	ERR_load_BIO_strings();
	SSL_load_error_strings();
	SSL_library_init();
	ctx = SSL_CTX_new(SSLv23_client_method());

	/* load the trust store */
	if (SSL_CTX_load_verify_locations(ctx, "ca.pem", NULL) == 0) {
		mlog("client.log", "Error loading certificate authoroty.\n");
		ERR_print_errors_fp(stderr);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}

	/* setup the connection and set the auto retry flag */
	bio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	/* set the host */
	char *host;
	asprintf(&host, "%s:%s", hostname, port);
	BIO_set_conn_hostname(bio, host);
	free(host);

	/* connect */
	if (BIO_do_connect(bio) <= 0) {
		mlog("client.log", "Error attempting to connect\n");
		ERR_print_errors_fp(stderr);
		BIO_free_all(bio);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}

	/* check the certificate */
	if (SSL_get_verify_result(ssl) != X509_V_OK) {
		fprintf(stderr, "Certificate verification error: %ld\n", 
				SSL_get_verify_result(ssl));
		BIO_free_all(bio);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}

	/* write bio and ctx to con and return success  */
	con->bio = bio;
	con->ctx = ctx;

	return 0;
}

int getMessage(BIO *bio, Message *target)
{
	NetworkMessage nm;
	int read = BIO_read(bio, &nm, sizeof(NetworkMessage));
	if (read <= 0) {
		mlog("client.log", "could not read openssl");
		return -1;
	}

	target->message_id = (Message_ID) nm.message_id;
	target->sum = ntohs(nm.sum);
	target->pin = ntohs(nm.pin);
	target->onetimecode = nm.onetimecode;
	target->card_number = ntohs(nm.card_number);
	return 0;
}

size_t sendMessage(BIO *bio, Message *msg)
{
	NetworkMessage toSend = {
		.message_id = (uint8_t) msg->message_id,
		.sum = htons(msg->sum),
		.pin = htons(msg->pin),
		.onetimecode = msg->onetimecode,
		.card_number = htons(msg->card_number)
	};

	return BIO_write(bio, &toSend, sizeof(NetworkMessage));
}

void printMessage(Message *m)
{
	printf("message_id: %d\n", m->message_id);
	printf("sum: %d\n", m->sum);
	printf("pin: %d\n", m->pin);
	printf("onetimecode: %d\n", m->onetimecode);
	printf("card_number: %d\n", m->card_number);
}

int sendNetworkString(BIO *bio, char *string)
{
	uint8_t length = strlen(string);
	int res;
	res = BIO_write(bio, &length, sizeof(length));
	if (res <= 0) {
		return -1;
	}

	res = BIO_write(bio, string, length);
	if (res <= 0) {
		return -1;
	}

	return 0;
}

int getNetworkString(BIO *bio, NetworkString *nstr)
{
	mlog("server.log", "getNetworkString");

	/* first receive the length */
	uint8_t length;
	int rec = BIO_read(bio, &length, sizeof(length));
	if (rec != sizeof(length)) {
		return -1;
	}

	mlog("server.log", "got length = %d", length);

	/* then receive the string */
	nstr->string = (char *) malloc(length + 1);
	rec = BIO_read(bio, nstr->string, length);
	if (rec <= 0) {
		free(nstr->string);
		return -1;
	}

	nstr->string_length = length;
	nstr->string[length] = '\0';

	mlog("server.log", "read %d bytes and got string '%s'", rec,
			nstr->string);

	return 0;
}

int password_callback(char *buf, int size, int rwflag, void *userdata)
{
    /* For the purposes of this demonstration, the password is "ibmdw" */

    printf("*** Callback function called\n");
    strcpy(buf, "ibmdw");
    return 1;
}

int start_server(char *port, void(*handle)(BIO*), int *sock)
{
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *bio, *abio, *out/*, *sbio*/;

	int (*callback)(char *, int, int, void *) = &password_callback;

	printf("Secure Programming with the OpenSSL API, Part 4:\n");
	printf("Serving it up in a secure manner\n\n");

	SSL_load_error_strings();
	ERR_load_BIO_strings();
	ERR_load_SSL_strings();
	OpenSSL_add_all_algorithms();

	printf("Attempting to create SSL context... ");
	ctx = SSL_CTX_new(SSLv23_server_method());
	if(ctx == NULL)
	{
		printf("Failed. Aborting.\n");
		return EXIT_FAILURE;
	}

	printf("\nLoading certificates...\n");
	SSL_CTX_set_default_passwd_cb(ctx, callback);
	if(!SSL_CTX_use_certificate_file(ctx, "certificate.pem", SSL_FILETYPE_PEM))
	{
		ERR_print_errors_fp(stdout);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}
	if(!SSL_CTX_use_PrivateKey_file(ctx, "private.key", SSL_FILETYPE_PEM))
	{
		ERR_print_errors_fp(stdout);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}

	printf("Attempting to create BIO object... ");
	bio = BIO_new_ssl(ctx, 0);
	if(bio == NULL)
	{
		printf("Failed. Aborting.\n");
		ERR_print_errors_fp(stdout);
		SSL_CTX_free(ctx);
		return EXIT_FAILURE;
	}

	printf("\nAttempting to set up BIO for SSL...\n");
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	abio = BIO_new_accept("4422");
	BIO_set_accept_bios(abio, bio);

	printf("Waiting for incoming connection...\n");

	if(BIO_do_accept(abio) <= 0)
	{
		ERR_print_errors_fp(stdout);
		SSL_CTX_free(ctx);
		BIO_free_all(bio);
		BIO_free_all(abio);
		return EXIT_FAILURE;
	}

	/* the server loop */
	while (true) {
		if (BIO_do_accept(abio) <= 0) {
			ERR_print_errors_fp(stdout);
			SSL_CTX_free(ctx);
			BIO_free_all(bio);
			BIO_free_all(abio);
			return EXIT_FAILURE;
		}

		out = BIO_pop(abio);

		if (!fork()) {
			if (BIO_do_handshake(out) <= 0) {
				printf("Handshake failed.\n");
				ERR_print_errors_fp(stdout);
				SSL_CTX_free(ctx);
				BIO_free_all(bio);
				BIO_free_all(abio);
				return EXIT_FAILURE;
			}

			handle(out);
			if (BIO_flush(out) <= 0) {
				printf("flushing failure");
			}

			BIO_free_all(out);
		}

		BIO_free_all(out);
	}

	BIO_free_all(bio);
	BIO_free_all(abio);

	SSL_CTX_free(ctx);

	return EXIT_SUCCESS;
}

