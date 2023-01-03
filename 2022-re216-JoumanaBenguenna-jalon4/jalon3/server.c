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
#include <assert.h>

#include "common.h"
#include "msg_struct.h"
#define BUF_SIZE 100 


typedef struct infos{
    int descriptor;
    char *pseudo;
    char *address;
    int port;
    struct infos *next;
}infos_client_t;


void modif(infos_client_t *new_client, char*pseudo, int sock){

}

void reinitialisation_case( struct pollfd fds[20], int i){
    fds[i].fd = -1;
    fds[i].events = 0;
    fds[i].revents = 0;

}

void echo_server(int sockfd, struct pollfd fds[20], int i, infos_client_t *new_client) {
    struct message msgstruct;
	char buff[MSG_LEN];
    memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
    char *message_to_send=buff;
	while (1) {
		// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
        memset(buff, 0, MSG_LEN);
        // Receiving structure
		if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			break;
		}
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
        // rec2 = recv(fds[i].fd, buffer, msgstruct . pld len , 0);
        // if (rec2 == −1){
        //     perror("Error while receiving message"); 
        //     exit (1);
        // }   

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);

        if(strcmp(buff, "./quit\n") ==0){
            printf("Deconnexion of a client \n");
            reinitialisation_case(fds, i);
            close(sockfd);
            break;
        }
        else if(msgstruct.type==NICKNAME_NEW){

            //strcpy(msgstruct.infos,new_client->pseudo);
            //while(new_client->next!=NULL){
                //if (sockfd==new_client->descriptor)
                //printf("le truc %d\n",new_client->descriptor);
               // printf("le truc %s\n",new_client->address);
                    new_client->pseudo=msgstruct.infos;
                    //pseudo1=new_client;
                    //printf("le truc %s\n",new_client->pseudo);
                //else
                    //new_client=new_client->next;
            
            //}
            
        }
        else if(msgstruct.type==NICKNAME_LIST){
            assert(new_client);
            memset(buff, 0, MSG_LEN);
            //strcpy(message_to_send,"oki ");
            // if(new_client->next!=NULL)
            //     printf("pas null %s\n", new_client->pseudo);
            // else{
            //     printf("null\n");

            // }
            
            //while(new_client->next!=NULL){
            // while(new_client->next!=NULL){
            //     if (sockfd==new_client->descriptor){
               // printf("le truc222 %s\n",new_client->pseudo);
                strcat(message_to_send,new_client->pseudo);
            //     printf("message to send %s", message_to_send);}
            // else
            //     new_client=new_client->next;
           //}    

        }
        printf("Received: %s", buff);
        // Sending structure (ECHO)
		if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
        // Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}

		printf("Message sent!\n");
      //  printf("le truc %s\n",new_client->pseudo);
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

    int max_cli= 20;

    infos_client_t *head_list= (infos_client_t *)malloc(sizeof(infos_client_t));
    head_list->descriptor=-1;
    head_list->port=0;
    head_list->pseudo="";
    head_list->address="";
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
                
                infos_client_t *new_client= (infos_client_t *)malloc(sizeof(infos_client_t));
                new_client->descriptor=fd;
                new_client->address=inet_ntoa(client_addr.sin_addr);
                new_client->port=ntohs(client_addr.sin_port);
                //new_client->pseudo="";
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
                echo_server(fds[i].fd, fds, i,head_list);    

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
