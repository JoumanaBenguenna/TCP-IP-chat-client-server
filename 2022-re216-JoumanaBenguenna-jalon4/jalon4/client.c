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

// global variable to not lose the nickname
char pseudo_final[128] = "";


void echo_client(int sockfd) {
    char *pseudo;
    int max_user=2;

    //initialization of the poll tab cases, first and second
    struct pollfd fds_c[max_user];
    fds_c[0].fd = STDIN_FILENO;
    fds_c[0].events = POLLIN;
    fds_c[0].revents = 0;

    fds_c[1].fd = sockfd;
    fds_c[1].events = POLLIN;
    fds_c[1].revents = 0;   

    // memory stuff
    struct message msgstruct;
	char buff[MSG_LEN];    
    memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);

    int s=0;    // a variable to know if the client has well used the /nick command
	printf("Message: ");
    printf("[Server] : please login with /nick <your pseudo>\n");

	while (1) {
        
        // opening poll function
        int r= poll(fds_c, max_user, -1);
        if(-1==r){
            perror("poll");
        }

        int i;
        for(i=0; i<max_user; i++){

            char *information;
            int data_to_be_read = fds_c[i].revents & POLLIN;

            if(fds_c[i].fd==STDIN_FILENO && data_to_be_read == POLLIN){   
                // the socket to send messages and structures is active

                memset(&msgstruct, 0, sizeof(struct message));
                memset(buff, 0, MSG_LEN);
                // Getting message from client
                if(s==0) // tant qu'un pseudo n'est pas entré on renvoie la demande
                    printf("[Server] : please login with /nick <your pseudo>\n");

                
                int n = 0;
                while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
                
                
                if(strstr(buff,"/nick") != NULL){ // Si la commande entrée est "/nick"
                    pseudo=strstr(buff,"/nick");  // On récupère dans pseudo, la chaine de caractère tapée par le client
                    pseudo+=strlen("/nick ");     // On récupère l'argument de la commande
                    pseudo[strlen(pseudo)-1] = '\0'; 	// on retire le backslach n		
                    memset(&msgstruct,0,sizeof(struct message));
                    
                    if(strstr(pseudo," ") != NULL){ // si le pseudo contient un espace
                        printf("[Server] : Your pseudo musn't have any space, please try again !\n");
                        continue;
                    }
                    else if(strlen(buff)>NICK_LEN+6){ //si le pseudo est trop long
                        printf("[Server] : Your pseudo too long, please try again !\n");
                        continue;
                    }
                    //si le pseudo contient un caractère spécial :
                    else if((strstr(pseudo,"=")!= NULL) | (strstr(pseudo,",")!= NULL) |(strstr(pseudo,"!")!= NULL) |(strstr(pseudo,"?")!= NULL) |(strstr(pseudo,":")!= NULL) ){
                        printf("[Server] : Your pseudo musn't have any special character, please try again !\n");
                        continue;
                    }
                    else{
                        // the pseudo is correct and acceptable
                        int i;
                        for(i=0;i<strlen(buff);i++){ // putting the pseudo collected in the global variable
                            pseudo_final[i]=pseudo[i];}

                        msgstruct.pld_len = strlen(buff);
                        msgstruct.type = NICKNAME_NEW;
                        strcpy(msgstruct.infos,pseudo_final);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        s++; // s=1 donc l'utilisateur se connecte pour la première fois
                        if (s==1) 
                            printf("[Server] : Welcome on the chat %s\n",pseudo);
                        if(s>1){  // si int s a été incrémenté + d'une fois alors l'utilisateur souhaite changer de pseudo
                            printf("[Server] : Your pseudo has been changed successfully %s\n",pseudo);    
                            s--;
                        }
                    }

                }        
                else if(s==0){ // si aucun pseudo n'a ete récuperé 
                    memset(&msgstruct,0,sizeof(struct message));
                    msgstruct.pld_len = strlen(buff);
                    msgstruct.type = ECHO_SEND;
                    strcpy(msgstruct.infos, "\0");
                    strcpy(msgstruct.nick_sender, "");
                

                }
                else if(s>0){ // Si nous avons enfin un  pseudo
                    
                    if(strstr(buff,"/whois") != NULL){ // /whois command
                        information=strstr(buff,"/whois");       //collecting the argument as in /nick command
                        information+=strlen("/whois ");              
                        information[strlen(information)-1] = '\0';    
                         
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = NICKNAME_INFOS;
                        strcpy(msgstruct.infos, information);
                    }            
                    
                    else if(strstr(buff,"/who") != NULL){ // /who command
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = NICKNAME_LIST;
                        strcpy(msgstruct.infos, "\0");
                    }
                    else if(strstr(buff,"/msgall") != NULL){  // /msgall command
                        information=strstr(buff,"/msgall");      //collecting the argument as in /nick command
                        information+=strlen("/msgall "); 
                        information[strlen(information)-1] = '\0'; 	
            
                        strcpy(buff,information);
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = BROADCAST_SEND;
                        strcpy(msgstruct.infos,"\0" );
                    }

                    else if(strstr(buff,"/msg") != NULL){ // /msg command
                        char destinataire[NICK_LEN];
                        information=strstr(buff,"/msg");
                        information+=strlen("/msg ");  //collecting the argument as in /nick command

                        int j=0;
                        while(information[j]!=' '){
                            destinataire[j]=information[j];
                            j++;}
                        destinataire[j]='\0';
                        information+=strlen(destinataire);
                        strcpy(buff,information);
                   
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = UNICAST_SEND;
                        strcpy(msgstruct.infos, destinataire);
                    }  
		            else if(strstr(buff,"/create") != NULL){  // /create command


                        information=strstr(buff,"/create");  //collecting the argument as in /nick command
                        information+=strlen("/create "); 
                        information[strlen(information)-1] = '\0'; 	


                        memset(&msgstruct,0,sizeof(struct message));
                        if(strstr(information," ") != NULL){ // si le nom contient un espace
                            printf("[Server] : Channel's name musn't have any space, please try again !\n");
                            continue;
                        }
                        else if(strlen(buff)>NICK_LEN+6){ //si le nom est trop long
                            printf("[Server] : Channel's name too long, please try again !\n");
                            continue;
                        }
                        // si le nom possède un caractère spécial
                        else if((strstr(information,"=")!= NULL) | (strstr(pseudo,",")!= NULL) |(strstr(pseudo,"!")!= NULL) |(strstr(pseudo,"?")!= NULL) |(strstr(pseudo,":")!= NULL) ){
                            printf("[Server] : Channel's name musn't have any special character, please try again !\n");
                            continue;
                        }
                        else{ // si le nom est acceptable
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = MULTICAST_CREATE;
                        strcpy(msgstruct.infos, information);
                        printf("[Server] : You have created channel %s\n",information);
                        }



		            }
                    else if(strstr(buff,"/channel_list") != NULL){  // /channel_list command
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = MULTICAST_LIST;
                        strcpy(msgstruct.infos, "\0");
                    }
                                    
                    else if(strstr(buff,"/join") != NULL){  // /join a channel command
                        information=strstr(buff,"/join");
                        information+=strlen("/join ");   //collecting the argument as in /nick command
                        information[strlen(information)-1] = '\0'; 		    
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = MULTICAST_JOIN;
                        strcpy(msgstruct.infos, information);
                    }
                    
                    else if(strstr(buff,"/quit ") != NULL){  // /quit a channel command
                        information=strstr(buff,"/quit");
                        information+=strlen("/quit ");       //collecting the argument as in /nick command
                        information[strlen(information)-1] = '\0'; 		    
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = MULTICAST_QUIT;
                        strcpy(msgstruct.infos, information);
                    } 

                    else{ // if non of the command, echo send
                        memset(&msgstruct,0,sizeof(struct message));
                        msgstruct.pld_len = strlen(buff);
                        strcpy(msgstruct.nick_sender, pseudo_final);
                        msgstruct.type = ECHO_SEND;
                        strcpy(msgstruct.infos, "\0");

                    }
                }
             
                // Sending structure
                if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
                    break;
                }
                // Sending message (ECHO)
                if (send(sockfd, buff, strlen(buff), 0) <= 0) {
                    break;
                }
                if(strcmp(buff, "/quit\n") ==0){
                    printf("You are disconnected from the server !\n");
                    break;
                }
                printf("Message sent!\n");
    


            }else if(fds_c[i].fd==sockfd  && data_to_be_read == POLLIN){
                // the socket to receive is active

                //memory stuff
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
                

                //printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
                if ((msgstruct.type == UNICAST_SEND) | (msgstruct.type == BROADCAST_SEND)){
                    printf("Received: [%s] : %s\n", msgstruct.nick_sender, buff);
                }
                else{ 
                    printf("Received: %s\n", buff);
                }
                memset(&msgstruct, 0, sizeof(struct message));
                memset(buff, 0, MSG_LEN);
                
            }
        }
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
