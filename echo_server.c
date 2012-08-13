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

#define ECHO_PORT 9999
#define BUF_SIZE 4096

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

void server_init() {


}

int main(int argc, char* argv[])
{
    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in addr, cli_addr;
    char buf[BUF_SIZE];

	fd_set master;
	fd_set read_fds;
	int fdmax;
	int i = 0;
	

    fprintf(stdout, "----- Echo Server -----\n");
    
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }
	/* Add to master set */
	FD_SET(sock,&master);
	fdmax = sock;

    /* finally, loop waiting for input and then write it back */
    while (1)
    {
		read_fds = master;
		if (select(fdmax + 1, &read_fds,NULL,NULL,NULL) == -1) {
			fprintf(stderr,"Error in select\n");
		}
		for (i = 0; i <= fdmax; i ++) {
			if (FD_ISSET(i, &read_fds)) { 
				if (i == sock) {
					//handle new connections
       				cli_size = sizeof(cli_addr);
       				if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                   						              &cli_size)) == -1) {
           				close(sock);
           				fprintf(stderr, "Error accepting connection.\n");
           				return EXIT_FAILURE;
       				}
					FD_SET(client_sock,&master);
					if (client_sock > fdmax) {
						fdmax = client_sock;
					}
				} else { //handle data from client
					readret = 0;
					readret = recv(i, buf, BUF_SIZE, 0);
					if(readret > 0) {
						if (send(i, buf, readret, 0) != readret) {
			            	close_socket(i);
							close_socket(sock);
							fprintf(stderr, "Error sending to client.\n");
							return EXIT_FAILURE;
						}
						fprintf(stdout,"Sent what we got \n");
						memset(buf, 0, BUF_SIZE);
					} else if (readret == 0) {  //client closed the socket
						close_socket(i);
						FD_CLR(i,&master);
					} else { //error case
           				close_socket(i);
           				close_socket(sock);
           				fprintf(stderr, "Error reading from client socket.\n");
           				return EXIT_FAILURE;
					}
				}
			}
		}
		/* close all client sockets 
       if (close_socket(client_sock))
       {
           close_socket(sock);
           fprintf(stderr, "Error closing client socket.\n");
           return EXIT_FAILURE;
       }
		*/
    }
    close_socket(sock);

    return EXIT_SUCCESS;
}
