#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>
#include "common.h"

#define BUF_SIZE 100 


typedef struct infos{
    int descriptor;
    char *address;
    int port;
    struct infos *next;
}infos_client;



void reinitialisation_case( struct pollfd fds[20], int i){
    fds[i].fd = -1;
    fds[i].events = 0;
    fds[i].revents = 0;

}

void echo_server(int sockfd, struct pollfd fds[20], int i) {
	char buff[MSG_LEN];
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
        // Deconnexion of a client with the ./quit command
        if(strcmp(buff, "./quit\n") ==0){
            printf("Deconnexion of a client \n");
            reinitialisation_case(fds, i);
            close(sockfd);
            break;
        }

        printf("Received: %s", buff);

        // Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}

		printf("Message sent!\n");
    break;
	}
}

int handle_bind(char *argv[]) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, argv[1], &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

void connexion_multiclient( int socket_ecoute){
    // Maximum client number
    int max_cli= 20; 
    // Head of the linked structures
    infos_client *head_list= (infos_client *)malloc(sizeof(infos_client));
    head_list->descriptor=-1;
    head_list->port=0;
    head_list->address="0.0.0.2";
    head_list->next=NULL;

    
    struct pollfd fds[max_cli];
    fds[0].fd = socket_ecoute;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    int k;
    for(k=1; k<max_cli; k++){
        fds[k].fd = -1;
        fds[k].events = 0;
        fds[k].revents = 0;
    }

    printf("Waiting for clients...\n");

    while(1){
        int r= poll(fds, max_cli, -1);
        if(-1==r){
            perror("poll");
        }   

        int i;
        for(i=0; i<max_cli; i++){
            // if active socket is Listen_fd socket
            int data_to_be_read = fds[i].revents & POLLIN;

            if(i==0 && data_to_be_read == POLLIN){
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                memset(&client_addr, 0,addr_len);

                int fd= accept(fds[i].fd,(struct sockaddr*)&client_addr, &addr_len);
        
                if (-1 == fd){
                    perror("Accept:");
                  
                }
                // Informations of the new client
                infos_client *new_client= (infos_client *)malloc(sizeof(infos_client));
                new_client->descriptor=fd;
                new_client->address=inet_ntoa(client_addr.sin_addr);
                new_client->port=ntohs(client_addr.sin_port);
                new_client->next=head_list;
                head_list=new_client;

 

                int j=0;
                while((fds[j].fd != -1) && (j<=max_cli))
                    j++;

                fds[j].fd=fd;
                fds[j].events = POLLIN; 
                fds[j].revents=0;
                
                printf("New client connected !\n");
                printf("Client port #: %d\n", ntohs(client_addr.sin_port));
            }

            if((i!=0) && (data_to_be_read != 0)){
                echo_server(fds[i].fd, fds, i);
                break;    
            }
        }
    }
}   





int main(int argc, char *argv[]) {
	
    int sfd; 

	sfd = handle_bind(argv);

	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		exit(EXIT_FAILURE);
	}
    
    connexion_multiclient(sfd);
    close(sfd);

	return EXIT_SUCCESS;
}
