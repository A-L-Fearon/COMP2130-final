#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
  
#define TRUE   		1
#define FALSE  		0
#define PORT 		60000
#define BUF_SIZE	1024
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30
#define GRP_WORK    2
#define GRP_FUN     3

struct client {
    int socket;
    char *alias;
    int group;
    int port_no;
    int available;
};



int main(int argc, char *argv[])
{
	int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    struct client clients[MAX_CLIENTS];

    char buffer[BUF_SIZE + 1];  //data buffer of 1K
      
    //set of socket descriptors
    fd_set readfds;
      
    //a message
    char *message = "Welcome to C chat\r\n";
    char *menu_options = "#1 - Group Chat\n2 - Peer to Peer\n";
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
        clients[i].socket = 0;
        clients[i].available = 0;
        // clients[i].alias = "user";
    }
      
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
     
    while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
  
        bzero(buffer, BUFFER_SIZE); 

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
         
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
            sd = client_socket[i];
            // sd = client.socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        //If something happened on the master socket , then 
        // its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            char prt[10];

            sprintf(prt, "%d", ntohs(address.sin_port));

            if ( read(new_socket, buffer, (BUF_SIZE - 1)) < 0)
            {
                printf("Error reading from socket\n");
                exit(0);
            }

            char *welcome_message = "Welcome to C Chat ";
              
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;

                    clients[i].socket = new_socket;
                    clients[i].port_no = *prt;
                    clients[i].alias = buffer;
                    
                    printf("Adding %s to list of sockets as %d\n", clients[i].alias, i);


                    // strcat(welcome_message, buffer);
                    // puts("herrrre");

                    //send new connection greeting message
                    if( send(new_socket, buffer, strlen(buffer), 0) != strlen(buffer) ) 
                    {
                        perror("send");
                    }

                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds)) 
            {
                bzero(buffer, BUF_SIZE); // resets buffer

                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    // close( sd );
                    client_socket[i] = 0;
                    clients[i].port_no = 0;
                }
                  
                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read
 
                    buffer[valread] = '\0';
                    // buffer[valread] = '\0';
                    // send(sd , buffer , strlen(buffer) , 0 );
                    // int j =  sizeof(buffer);
                    
                    // puts(buffer);

                    if (buffer[0] == '@') // peer to peer options
                    {
                        if (buffer[1] == '@') // retrieves all users
                        {
                            int j = 0;
                            char client_list[BUF_SIZE] = "Available Clients\nCommand\tUser\n";

                            for (j = 0; j < max_clients; j++)
                            {

                                if (clients[j].socket != 0)
                                {
                                    char client_id[2];
                                    char *sig, *tab, *nl;
                                    
                                    sig = "@";
                                    tab = "\t";
                                    nl = "\n";

                                    // if (j < 10)
                                    // {
                                    //     client_id[0] = j;
                                    // }
                                    // else
                                    // {
                                    //     client_id[0] = j / 10;
                                    //     client_id[1] = j % 10;
                                    // }

                                    // sprintf(client_id,"%d", j);

                                    puts("e");
                                    // strcat(client_list, sig);
                                    puts("a");
                                    // strcat(client_list, client_id);
                                    puts("b");
                                    // strcat(client_list, tab);

                                    // strcat(client_list, clients[j].alias);
                                    // strcat(client_list, nl);

                                }      
                                write(sd, client_list, sizeof(client_list));
                            }
                            
                        }
                        else if (buffer[2] == '+')
                        {
                            int j, peerNum = 0;
                            char num[4];

                            for (j = 2; j < strlen(buffer); j++)
                            {
                                if (buffer[j] != '@');
                                {
                                    num[peerNum] = buffer[j];
                                    peerNum++;
                                }
                                
                                // send request to connect to client
                            }
                        }
                    }
                    else if (buffer[0] == '#') // broadcast
                    {
                        if (buffer[1] == '#') // gets broadcast option
                        {

                        }
                        else if (buffer[1] == '+') // join the group
                        {

                        }
                        else if (buffer[1] == '-') // leave group
                        {

                        }
                        else // broadcase to the specified group the preceeding message
                        {
                            //int grp = atoi(buffer[1]); 
                        }
                    }

                    //if ((valread = read( sd , buffer, 1024)) == 'hello'){
                    //if(strcmp(buffer,"hello") == 0){
                    //	send(sd, "worked", strlen(buffer) , 0 );
                    //}
                    int test = (strcmp(buffer,"hello"));
                    if(send(sd , buffer , strlen(buffer), 0 ) != strlen(buffer)){
                    	puts("failed");
                    }
                    puts(buffer);
                    //send(sd , buffer , strlen(buffer) , 0 );
                    //puts("hey");
                    // if (buffer[0] == '#') // set username and configurations
                    // {
                    //     if (buffer[1] == '0')
                    //     {
                    //         int count = 0;

                    //         for (j = 2; j < strlen(buffer); j++)
                    //         {
                    //             clients[i].alias[count] = buffer[j];   
                    //         }

                    //         if (strcmp(clients[i].alias, "user") == 0)
                    //         {
                    //             char user_count[2];
                    //             sprintf(user_count, "%d",i);
                    //             strcat(clients[i].alias, user_count);
                    //         }

                    //         // the user is now connected, return their username
                    //         printf("%s", clients[i].alias);    
                    //         send(clients[i].socket, clients[i].alias , strlen(clients[i].alias ), 0);
                    //     }
                    // }
                }
            }
        }
    }
      
    return 0;
}