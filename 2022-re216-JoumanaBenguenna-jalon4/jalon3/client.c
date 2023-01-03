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
char pseudo_final[128] = "";


//char pseudo[128] = ""; 

// char asking_for_nickname(int s){

//     if(s==0) // tant qu'un pseudo n'est pas entré on renvoie la demande
//         printf("[Server] : please login with /nick <your pseudo>\n");
// 	n = 0;
// 	while ((buff[n++] = getchar()) != '\n') {} 

//     if(strstr(pseudo," ") != NULL){
//         printf("[Serveur] : Your pseudo musn't have any space, please try again !\n");
//         continue;
//             }        

// }

char *pseudo;
void echo_client(int sockfd) {
    struct message msgstruct;
	char buff[MSG_LEN];    
    memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);
    int s=0;
	//int n;
    
	while (1) {
		// Cleaning memory
        memset(&msgstruct, 0, sizeof(struct message));
        memset(buff, 0, MSG_LEN);
        char *information;
        
		// Getting message from client
         if(s==0) // tant qu'un pseudo n'est pas entré on renvoie la demande
             printf("[Server] : please login with /nick <your pseudo>\n");

		printf("Message: ");
		int n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
        
        
		if(strstr(buff,"/nick") != NULL){ // Si la commande entrée est "/nick"
            pseudo=strstr(buff,"/nick");
			pseudo+=strlen("/nick "); // On récupère l'argument de la commande c'est à dire le pseudo
            pseudo[strlen(pseudo)-1] = '\0'; 	// on enlève le backslach n		
            memset(&msgstruct,0,sizeof(struct message));
            
            if(strstr(pseudo," ") != NULL){ // si le pseudo contient un espace
                printf("[Serveur] : Your pseudo musn't have any space, please try again !\n");
                continue;
            }
            else if(strlen(buff)>NICK_LEN+6){ //si le pseudo est trop long
                printf("[Serveur] : Your pseudo too long, please try again !\n");
                continue;
            }
            else{
                for(int i=0;i<strlen(buff);i++){
                    pseudo_final[i]=pseudo[i];}

                msgstruct.pld_len = strlen(buff);
                msgstruct.type = NICKNAME_NEW;
		        strcpy(msgstruct.infos,pseudo_final);
		        strcpy(msgstruct.nick_sender, pseudo_final);
                s++;
                if (s==1) 
                    printf("[Serveur] : Welcome on the chat %s\n",pseudo);
                if(s>1){  // si int s a été incrémenté c'est que l'utilisateur souhaite changer de pseudo
                    printf("[Serveur] : Your pseudo has been changed successfully %s\n",pseudo);    
                    s--;
                }
            }

        }        
        else if(s==0){ // si aucun pseudo n'a ete récuperé 
            memset(&msgstruct,0,sizeof(struct message));
            msgstruct.pld_len = strlen(buff);
		    msgstruct.type = ECHO_SEND;
		    strcpy(msgstruct.infos, "h\0");
		    strcpy(msgstruct.nick_sender, "");
            //printf("s %i\n",s);

        }
        else if(s>0){ // Si nous avons enfin un  pseudo
            //printf("pseudo boucle: %s\n",pseudo);
            
            if(strstr(buff,"/whois") != NULL){
                information=strstr(buff,"/whois");
			    information+=strlen("/whois "); // On récupère l'argument de la commande c'est à dire le pseudo
                information[strlen(information)-1] = '\0'; 	// on enlève le backslach n		    
                memset(&msgstruct,0,sizeof(struct message));
		        msgstruct.pld_len = strlen(buff);
		        strcpy(msgstruct.nick_sender, pseudo_final);
		        msgstruct.type = NICKNAME_INFOS;
		        strcpy(msgstruct.infos, information);
            }            
            
            else if(strstr(buff,"/who") != NULL){
                memset(&msgstruct,0,sizeof(struct message));
		        msgstruct.pld_len = strlen(buff);
		        strcpy(msgstruct.nick_sender, pseudo_final);
		        msgstruct.type = NICKNAME_LIST;
		        strcpy(msgstruct.infos, "\0");
            }
            else if(strstr(buff,"/msgall") != NULL){    
                memset(&msgstruct,0,sizeof(struct message));
		        msgstruct.pld_len = strlen(buff);
		        strcpy(msgstruct.nick_sender, pseudo_final);
		        msgstruct.type = BROADCAST_SEND;
		        strcpy(msgstruct.infos,"\0" );
            }

            else if(strstr(buff,"/msg") != NULL){
                char *destinataire;
                //destinataire=strstr(buff,"/msg");
                //information=strstr(buff,"/msg");
			    //information+=strlen("/msg "); // On récupère l'argument de la commande c'est à dire le pseudo
                information=strstr(buff,"/msg");
			    information+=strlen("/msg "); // On récupère l'argument de la commande c'est à dire le pseudo
                
                information[strlen(information)-1] = '\0'; 	// on enlève le backslach n		
                int j=0;
                while(information[j]!=' '){
                    destinataire[j]=information[j];
                    j++;}

                printf("destinataire: %s",destinataire);
                   
                memset(&msgstruct,0,sizeof(struct message));
		        msgstruct.pld_len = strlen(buff);
		        strcpy(msgstruct.nick_sender, pseudo_final);
		        msgstruct.type = UNICAST_SEND;
		        strcpy(msgstruct.infos, destinataire);
            }            

            else{
                memset(&msgstruct,0,sizeof(struct message));
		        msgstruct.pld_len = strlen(buff);
		        strcpy(msgstruct.nick_sender, pseudo_final);
		        msgstruct.type = ECHO_SEND;
		        strcpy(msgstruct.infos, "\0");
                //printf("s sup %i\n",s);
			    // strcpy(msgstruct.infos, "\0");
            }
        }
        //printf("pseudo apres affectation: %s",pseudo);
		// Sending structure
        //printf("type avant send: %u",msgstruct.type);
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
       //printf("pseudo envoyé: %s\n",pseudo);
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
        
        printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("Received: %s\n", buff);
        
        //memset(&msgstruct, 0, sizeof(struct message));
		//memset(buff, 0, MSG_LEN);
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
