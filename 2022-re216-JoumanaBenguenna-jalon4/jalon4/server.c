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
#include <time.h>

#include "common.h"
#include "msg_struct.h"


// STRUCTURE CLIENT 
typedef struct infos{
    int descriptor;
    char pseudo[NICK_LEN];
    char address[64];
    uint16_t port;
    struct infos *next;
    char time[64];
    char channel[NICK_LEN];
}infos_client_t;

// STRUCTURE SALON
typedef struct channels{
    char name_channel[NICK_LEN];
    struct channels *nextc;
}channels_t;


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

    //initialization of the head channels structure
    channels_t *head_channel= (channels_t *)malloc(sizeof(channels_t));
    strcpy(head_channel->name_channel,"");
    head_channel->nextc=NULL;
    
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

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
        printf("Received: %s", buff);
        

        //====================> The client wants to quit the chat
        if(strcmp(buff, "/quit\n") ==0){
            printf("Deconnexion of a client \n");
            reinitialisation_case(fds, i);

            close(sockfd);
            break;
    
        }else if(msgstruct.type==NICKNAME_NEW){
            strcpy(new_client->pseudo,msgstruct.infos);
        
        
        //====================> the clients wants the list of the connected users
        }else if(msgstruct.type==NICKNAME_LIST){
            memset(buff, 0, MSG_LEN);
            strcat(buff,"[Server] : Online users are \n");
            while(new_client->next!=NULL){
                    
                strcat(buff,"               -");
                strcat(buff,new_client->pseudo);
                strcat(buff,"\n");
                new_client=new_client->next;
            }    
        
        
        //======================> the client wants information about a specific user
        }else if(msgstruct.type==NICKNAME_INFOS){
            memset(buff, 0, MSG_LEN);
            char collect_info[NICK_LEN];
            strcpy(collect_info,msgstruct.infos);

            //research in the linked list of the wanted client
            while(new_client->next){

                if(strcmp(collect_info,new_client->pseudo)==0){
                    sprintf(buff,"[Server] : The client with %i port, named ",new_client->port);
                    strcat(buff,new_client->pseudo);
                    strcat(buff," is connected with IP address " );
                    strcat(buff,new_client->address);
                    strcat(buff," since ");
                    strcat(buff,new_client->time);
                    strcat(buff,"\n");
                    break;
                }else{
                    new_client=new_client->next;
                }
            }
        
        
        //=======================> the client wants to send a message to all the users
        }else if(msgstruct.type==BROADCAST_SEND){
            
            //send to all the users except to the one who send the message
            while(new_client->next!=NULL){
                if(new_client->descriptor!= sockfd){
                    if (send(new_client->descriptor, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			            break;
		            }
                    if (send(new_client->descriptor, buff, strlen(buff), 0) <= 0) {
			            break;
		            }
                    
                }
                new_client=new_client->next;
            }
            break;
            

        //=========================> the client sends a message to a specific user
        }else if(msgstruct.type==UNICAST_SEND){
	        int collect=0; // variable to know if the user targeted exists
            while(new_client->next!=NULL){
                if(strcmp(new_client->pseudo,msgstruct.infos)==0){
		            collect++;
                    if (send(new_client->descriptor, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			            break;
		            }
                    if (send(new_client->descriptor, buff, strlen(buff), 0) <= 0) {
			            break;
		            }
                    
                }
                new_client=new_client->next;
            }
            //============§§§§§§> code qui ne fais pas effet :
	        if(collect==0){ // tell to the client that the user wanted does not exists
	            memset(buff, 0, MSG_LEN);
	            strcat(buff, "[Server] : This user does not exist");
	            if (send(new_client->descriptor, &msgstruct, sizeof(msgstruct), 0) <= 0) {
		            break;
		        }
                if (send(new_client->descriptor, buff, strlen(buff), 0) <= 0) {
		            break;
		        }
	        }
            break;
            //============§§§§§>
        

        //==========================> the client creates a channel   
        }else if(msgstruct.type==MULTICAST_CREATE){

            channels_t *new_channel= (channels_t *)malloc(sizeof(channels_t));
            strcpy(new_channel->name_channel,msgstruct.infos);
            new_channel->nextc=head_channel;
            head_channel=new_channel;
            strcpy(new_client->channel,msgstruct.infos);


        //=========================> the client wants the list of the opened channels   
        }else if(msgstruct.type==MULTICAST_LIST){
            memset(buff, 0, MSG_LEN);
            strcat(buff,"[Server] : Active channels are \n");

        //===§§§§§> code juste mais souci d'initialisation de maillon salon 
            // while(new_channel->nextc!=NULL){

            //     strcat(buff,"               -");
            //     strcat(buff,new_channel->name_channel);
            //     strcat(buff,"\n");
            //     new_channel=new_channel->nextc;
            // }   
        //====§§§§§>      

            while(new_client->next!=NULL){

                strcat(buff,"               -");
                strcat(buff,new_client->channel);
                strcat(buff,"\n");
                new_client=new_client->next;
            }

        //==========================> the client want to join a specific channel
        }else if(msgstruct.type==MULTICAST_JOIN){
            memset(buff, 0, MSG_LEN);
            strcpy(new_client->channel,msgstruct.infos);
            while(new_client->next!=NULL){
                if( (strcmp(new_client->channel,msgstruct.infos)==0) | (strcmp(new_client->pseudo,msgstruct.nick_sender)!=0)   ){
                    strcat(buff,new_client->pseudo);
                    strcat(buff," has joined the channel");
                    if (send(new_client->descriptor, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			            break;
		            }
                    if (send(new_client->descriptor, buff, strlen(buff), 0) <= 0) {
			            break;
		            }
                    
                }
                new_client=new_client->next;                

            }break;



        }//=========================> the client want to quit a channel   
        else if(msgstruct.type==MULTICAST_QUIT){
            memset(buff, 0, MSG_LEN);
            strcpy(new_client->channel,"");
            while(new_client->next!=NULL){
                if((strcmp(new_client->channel,msgstruct.infos)==0) | (strcmp(new_client->pseudo,msgstruct.nick_sender)!=0) ){
                    strcat(buff,new_client->pseudo);
                    strcat(buff," has left the channel");
                    if (send(new_client->descriptor, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			            break;
		            }
                    if (send(new_client->descriptor, buff, strlen(buff), 0) <= 0) {
			            break;
		            }
                    
                }
                new_client=new_client->next;                

            }break;
        }          

        // Sending structure 
		if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			break;
		}
        // Sending message
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
    // maximum of clients
    int max_cli= 20;


    // initialization of the head list
    infos_client_t *head_list= (infos_client_t *)malloc(sizeof(infos_client_t));
    head_list->descriptor=-1;
    head_list->port=0;
    strcpy(head_list->pseudo,"");
    strcpy(head_list->address,"");
    head_list->next=NULL;

    // initialization of the listening socket
    struct pollfd fds[max_cli];
    fds[0].fd = socket_ecoute;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    // initialization of the others cases of the poll tab
    int k;
    for(k=1; k<max_cli; k++){
        fds[k].fd = -1;
        fds[k].events = 0;
        fds[k].revents = 0;
    }

    printf("Waiting for clients...\n");

    while(1){
        // opening poll function
        int r= poll(fds, max_cli, -1);
        if(-1==r){
            perror("poll");
        }   

        int i;
        for(i=0; i<max_cli; i++){
            // if active socket is the listening socket
            int data_to_be_read = fds[i].revents & POLLIN;
            
            if(i==0 && data_to_be_read == POLLIN){
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                memset(&client_addr, 0,addr_len);
                
                int fd= accept(fds[i].fd,(struct sockaddr*)&client_addr, &addr_len);
        
                if (-1 == fd){
                    perror("Accept:");
                  
                }
                
		            // collecting the data about the client 
                time_t now;
                time(&now); //collect the hour and date when the client has been connected
                infos_client_t *new_client=(infos_client_t *) malloc(sizeof(infos_client_t));
                new_client->descriptor=fd;
                strcpy(new_client->address,inet_ntoa(client_addr.sin_addr));
                new_client->port=ntohs(client_addr.sin_port);
                new_client->next=head_list;
		strcpy(new_client->time,ctime(&now));
                head_list=new_client;

 

                int j=0;
                while((fds[j].fd != -1) && (j<=max_cli))
                    j++;

                fds[j].fd=fd;
                fds[j].events = POLLIN; 
                fds[j].revents=0;
                
                printf("New client connected !\n");
                printf("Client port #: %d\n", ntohs(client_addr.sin_port));
                printf("Client address #: %s\n", inet_ntoa(client_addr.sin_addr));


            }

            if((i!=0) && (data_to_be_read != 0)){
                // if dialogue socket is active
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
