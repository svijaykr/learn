/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_parser->h"

#define ECHO_PORT 9999
#define BUF_SIZE 4096

#define MAX_HEADERS 10
#define MAX_ELEMENT_SIZE 500

/* Code copied from http-parser/test.c */

typedef struct {
	const char *name;
	              //for debugging purposes
	const char *raw;
	enum http_parser_type type;
	int	method;
	int	status_code;
	char request_path[MAX_ELEMENT_SIZE];
	char request_uri[MAX_ELEMENT_SIZE];
	char fragment[MAX_ELEMENT_SIZE];
	char query_string[MAX_ELEMENT_SIZE];
	char body[MAX_ELEMENT_SIZE];
	int	num_headers;
	enum {
		NONE = 0, FIELD, VALUE
	} last_header_element;
	char headers [MAX_HEADERS][2][MAX_ELEMENT_SIZE];
	int	should_keep_alive;
} message_t;

http_parser parser;
message_t *message;

int
request_path_cb (http_parser *_, const char *p, size_t len)
{
    strncat(message->request_path, p, len);
    return 0;
}

int
request_uri_cb (http_parser *_, const char *p, size_t len)
{
    strncat(message->request_uri, p, len);
    return 0;
}

int
query_string_cb (http_parser *_, const char *p, size_t len)
{
    strncat(message->query_string, p, len);
    return 0;
}

int
fragment_cb (http_parser *_, const char *p, size_t len)
{
    strncat(message->fragment, p, len);
    return 0;
}

int
header_field_cb (http_parser *_, const char *p, size_t len)
{
    struct message *m = message;

    if (m->last_header_element != FIELD)
      m->num_headers++;

    strncat(m->headers[m->num_headers-1][0], p, len);

    m->last_header_element = FIELD;

    return 0;
}

int
header_value_cb (http_parser *_, const char *p, size_t len)
{
    struct message *m = message;

    strncat(m->headers[m->num_headers-1][1], p, len);

    m->last_header_element = VALUE;

    return 0;
}

int
body_cb (http_parser *_, const char *p, size_t len)
{
    strncat(message->body, p, len);
 // printf("body_cb: '%s'\n", requests[num_messages].body);
    return 0;
}

int
message_complete_cb (http_parser *parser)
{
    message->method = parser->method;
    message->status_code = parser->status_code;

    return 0;
}

int
message_begin_cb (http_parser *_)
{
    return 0;
}

void
parser_init (http_parser *parser, int client_socket, enum http_parser_type type)
{


    http_parser_init(&parser, type);
    message_t *message = (message_t *) malloc(sizeof(struct message));

    memset(message, 0, sizeof message);

    parser->on_message_begin     = message_begin_cb;
    parser->on_header_field      = header_field_cb;
    parser->on_header_value      = header_value_cb;
    parser->on_path              = request_path_cb;
    parser->on_uri               = request_uri_cb;
    parser->on_fragment          = fragment_cb;
    parser->on_query_string      = query_string_cb;
    parser->on_body              = body_cb;
    parser->on_headers_complete  = NULL;
    parser->on_message_complete  = message_complete_cb;

    parser->data = client_socket;
}
int 
close_socket(int sock)
{
	if (close(sock)) {
		fprintf(stderr, "Failed closing socket.\n");
		return 1;
	}
	return 0;
}

void 
server_init()
{


}

int 
main(int argc, char *argv[])
{
	int	sock,client_sock;
	ssize_t	readret;
	socklen_t cli_size;
	struct sockaddr_in addr, cli_addr;
	char buf[BUF_SIZE];

	fd_set master;
	fd_set read_fds;
	int	fdmax;
	int	i = 0;


	fprintf(stdout, "----- Echo Server -----\n");

	/* all networked programs must create a socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Failed creating socket.\n");
		return EXIT_FAILURE;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ECHO_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	/*
	 * servers bind sockets to ports---notify the OS they accept
	 * connections
	 */
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		close_socket(sock);
		fprintf(stderr, "Failed binding socket.\n");
		return EXIT_FAILURE;
	}
	if (listen(sock, 5)) {
		close_socket(sock);
		fprintf(stderr, "Error listening on socket.\n");
		return EXIT_FAILURE;
	}
	/* Add to master set */
	FD_SET(sock, &master);
	fdmax = sock;

	/* finally, loop waiting for input and then write it back */
	while (1) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			fprintf(stderr, "Error in select\n");
		}
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == sock) {
					//handle new connections
						cli_size = sizeof(cli_addr);
					if ((client_sock = accept(sock, (struct sockaddr *)&cli_addr,
							&cli_size)) == -1) {
						close(sock);
						fprintf(stderr, "Error accepting connection.\n");
						return EXIT_FAILURE;
					}
					FD_SET(client_sock, &master);
					if (client_sock > fdmax) {
						fdmax = client_sock;
					}
				} else {
					//handle data from client
						readret = 0;
					readret = recv(i, buf, BUF_SIZE, 0);
					if (readret > 0) {
                        http_parser *parser = (http_parser *) malloc(sizeof(http_parser));
                        parser_init(parser,i,HTTP_REQUEST);
                        http_parser_execute(parser,buf,readret);
						if (send(i, buf, readret, 0) != readret) {
							close_socket(i);
							close_socket(sock);
							fprintf(stderr, "Error sending to client.\n");
							return EXIT_FAILURE;
						}
						fprintf(stdout, "Sent what we got \n");
						memset(buf, 0, BUF_SIZE);
					} else if (readret == 0) {
						//client closed the socket
							close_socket(i);
						FD_CLR(i, &master);
					} else {
						//error case
							close_socket(i);
						close_socket(sock);
						fprintf(stderr, "Error reading from client socket.\n");
						return EXIT_FAILURE;
					}
				}
			}
		}
		/*
		 * close all client sockets if (close_socket(client_sock)) {
		 * close_socket(sock); fprintf(stderr, "Error closing client
		 * socket.\n"); return EXIT_FAILURE; }
		 */
	}
	close_socket(sock);

	return EXIT_SUCCESS;
}
