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
#include "msg_struct.h"
#include "common.h"

#define BUF_SIZE 100

void echo_client(int sockfd) {
    struct message msgstruct;
	char buff[MSG_LEN];
    int s=0;
	int n;
	while (1) {
		// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
        char *pseudo;
        if(s==0) // tant qu'un pseudo n'est pas entré on renvoie la demande
            printf("[Server] : please login with /nick <your pseudo>\n");

		printf("Message: ");
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

        
		if(strstr(buff,"/nick") != NULL){ // Si la commande entrée est "/nick"
            pseudo=strstr(buff,"/nick");
			pseudo+=strlen("/nick "); // On récupère l'argument de la commande c'est à dire le pseudo
            pseudo[strlen(pseudo)-1] = '\0'; 	// on enlève le backslach n		
            memset(&msgstruct,0,sizeof(struct message));
		    msgstruct.type = NICKNAME_NEW;
		    strcpy(msgstruct.infos,pseudo);
		    strcpy(msgstruct.nick_sender, pseudo);
            s++;                            // si on a recupérer le pseudo, on n'affiche plus la demande d'utilisation de nick du serveur
            printf("[Serveur] : Welcome on the chat %s\n",pseudo);
        }
        else{ // si aucun pseudo n'a ete récuperé 
            memset(&msgstruct,0,sizeof(struct message));
		    msgstruct.type = ECHO_SEND;
		    strcpy(msgstruct.infos, "\0");
		    strcpy(msgstruct.nick_sender, "");

        }
        // Si nous avons enfin un  pseudo
        if((strstr(buff,"/nick") == NULL) && (s>0)){
            memset(&msgstruct,0,sizeof(struct message));
		    msgstruct.pld_len = strlen(buff);
		    strcpy(msgstruct.nick_sender, pseudo);
		    msgstruct.type = ECHO_SEND;
		    strcpy(msgstruct.infos, "\0");
        }
		// Sending structure
		if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
        if(strcmp(buff, "./quit\n") ==0){
            printf("You are disconnected from the server !\n");
            break;
        }
		printf("Message sent!\n");
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
        
        printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("Received: %s", buff);
        
	}
}

int handle_connect(char *argv[]) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(argv[1], argv[2], &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char *argv[]) {
	int sfd;
	sfd = handle_connect(argv);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}
