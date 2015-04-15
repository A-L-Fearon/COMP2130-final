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
    char alias[30];
    int groups[2];
    int port_no;
    int available;
};



int main(int argc, char *argv[])
{
	int opt = TRUE, online_count = 0;
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
        clients[i].groups[0] = 0;
        clients[i].groups[1] = 0;
        clients[i].available = -1; // unreachable
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
      
    //bind the socket to localhost, port 60000
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
                    strcpy(clients[i].alias, buffer);
                    clients[i].available = 0; // the client is availabe by default
                    
                    printf("Adding %s to list of sockets as %d\n", clients[i].alias, i);

                    //send new connection greeting message
                    if( send(new_socket, buffer, strlen(buffer), 0) != strlen(buffer) ) 
                    {
                        perror("send");
                    }

                    break;
                }
            }

            online_count++;
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
                    clients[i].socket = 0;
                    clients[i].groups[0] = 0;
                    clients[i].groups[1] = 0;
                    clients[i].available = -1; // the client does not exist

                    online_count--;
                }
                  
                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read

                    buffer[valread] = '\0';

                    // buffer[valread] = '\0';
                    // send(sd , buffer , strlen(buffer) , 0 );
                    // int j =  sizeof(buffer);
                    
                    puts("here");
                    int test = (strcmp(buffer,"test"));
                    printf("%d <-- \n", test);
                    printf("%d truth \n", buffer[0] == 'x');
                    if (buffer[0] == '@') // peer to peer options
                    {
                        if (buffer[1] == '@') // retrieves all available users
                        {
                            char client_list[BUF_SIZE];

                            if (online_count == 1) // Only one user is online 
                            {
                                strcpy(client_list, "No Peers Available\n");
                            }
                            else
                            {                            
                                int j = 0;
                                
                                strcpy(client_list, "Available Clients\n\tCommand\tUser\n");

                                for (j = 0; j < max_clients; j++)
                                {

                                    if (clients[j].port_no != 0 && clients[j].available == 0)
                                    {
                                        char client_id[2];
                                        
                                        sprintf(client_id, "%d", j);
                                        
                                        // creates list to display
                                        strcat(client_list, "\t"); 
                                        strcat(client_list, "@+");
                                        strcat(client_list, client_id);
                                        strcat(client_list, "\t"); 
                                        strcat(client_list, clients[j].alias);
                                        strcat(client_list, "\n");
                                    }      
                                }
                            }
                            if(send(sd , client_list , strlen(client_list), 0 ) != strlen(client_list))
                            {
                                puts("failed");
                            }
                        }
                        else if (buffer[1] == '+') // attempts to create new peer<->peer connection
                        {
                            int j = 2, index, count = 0;
                            char client_index[2], client_listening[10], response[BUF_SIZE];

                            int size = strlen(buffer);
                            printf("Sent = %s and its length is %d\n", buffer, size );

                            // gets the index of the client to connect to from the server
                            while(buffer[j] != '|')
                            {                                
                                client_index[count] = buffer[j];
                                count++;
                                j++; 
                            }

                            // converts the users index from string to integer
                            index = atoi(client_index);

                            puts("here");
 
                            if (clients[index].available == -1) // checks if the client exists
                            {
                               strcpy(response, "Client does not exist (enter @@ for list of clients)\n");

                               if(send(sd, response , strlen(response), 0 ) != strlen(response))
                                {
                                    puts("failed");
                                }
                                puts("here-here-here");
                            }
                            else if(clients[index].available > 0) // checks if client is available
                            {
                                strcpy(response, "Client is currently unavailable, Please try again later\n");

                                if(send(sd, response , strlen(response), 0 ) != strlen(response))
                                {
                                    puts("failed");
                                }
                                puts("server-server");
                            }
                            else // attempts to contact client
                            {
                                char index_to_string[2], port_to_string[10];

                                count = 0;
                                j++; // index of the port number in the buffer

                                // retrieves the port number the client is listening on

                                while (buffer[j] != '|')
                                {
                                    puts("attmps");
                                    client_listening[count] = buffer[j];
                                }

                                sprintf(index_to_string, "%d", i);

                                strcpy(response, "@+");                                
                                strcat(response, index_to_string);
                                strcat(response, "|");
                                strcat(response, port_to_string);
                                strcat(response, "|");

                                if(send(clients[index].socket, response , strlen(response), 0 ) != strlen(response))
                                {
                                    puts("failed");
                                }
                                clients[i].available = 1; // client is now 'busy'
                            }
                            puts(response);

                        }
                        else if (buffer[1] == '-') // rejects peer connections
                        {

                        }
                    }
                    else if (buffer[0] == '#') // broadcast
                    {
                    	puts("group entry");
                        char group_message[BUF_SIZE];
                        
                        strcpy(group_message, "GROUP OPTIONS\n\tCommand\tOption\n");

                        if (buffer[1] == '#') // gets broadcast option
                        {
                        	puts("group level 2");
                            if (clients[i].groups[0] == 0)
                            {
                            	puts("next");
                                strcat(group_message, "\t#+1\tJoin Fun Group\n");
                                puts("after");
                            }
                            else
                            {
                                strcat(group_message, "\t#-1\tLeave Fun Group\n\n");
                            }
                            puts("group 2nd if");
                            if (clients[i].groups[1] == 0)
                            {
                                strcat(group_message, "\t#+2\tJoin Work Group\n");
                            }
                            else
                            {                                
                                strcat(group_message, "\t#+2\tJoin Work Group\n\n");
                            }
                            puts("group 3rd if");
                            if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                            {
                                puts("failed");
                            }
                            puts("group end");
                        }
                        else if (buffer[1] == '+') // join the groups
                        {
                            char group_message[BUF_SIZE];
                            
                            if (buffer[2] == '1')
                            {
                                if (clients[i].groups[0] == 0) // Fun Group
                                {
                                    clients[i].groups[0] = 1;
                                    strcpy(group_message, "Joined Fun Group");
                                }
                                else
                                {
                                    strcpy(group_message, "Already in the Fun Group");
                                }
                            }
                            else if (buffer[2] == '2') // Work Group
                            {
                                if (clients[i].groups[1] == 0)
                                {
                                    clients[i].groups[1] = 1;
                                    strcpy(group_message, "Joined Work Group");
                                }
                                else
                                {
                                    strcpy(group_message, "Already in the Work Group");
                                }
                            }
                            else
                            {
                                strcpy(group_message, "Invalid Option");
                            }

                            if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                            {
                                puts("failed");
                            }
                        }
                        else if (buffer[1] == '-') // leave group
                        {
                            char *group_message;
                            
                            if (buffer[2] == '1')
                            {
                                if (clients[i].groups[0] != 0) // Fun Group
                                {
                                    clients[i].groups[0] = 0;
                                    group_message = "Left Fun Group";
                                }
                                else
                                {
                                    group_message = "You aren't in the Fun Group";
                                }
                            }
                            else if (buffer[2] == '2') // Work Group
                            {
                                if (clients[i].groups[1] != 0)
                                {
                                    clients[i].groups[1] = 0;
                                    group_message = "Joined Work Group";
                                }
                                else
                                {
                                    group_message = "You aren't in the Work Group";
                                }
                            }
                            else
                            {
                                group_message = "Invalid Option";
                            }

                            if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                            {
                                puts("failed");
                            }
                        }
                        else if (buffer[1] == '=')// broadcase to the specified group the preceeding message
                        {
                            int j, index = 0;
                            char broadcast_message[BUFFER_SIZE];
                            char *group_message;

                            group_message = "You arent in this group\n";

                            if (buffer[2] == '1') // Fun Group
                            {
                                if (clients[i].groups[0] == 0) // Not in the group
                                {
                                    if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                                    {
                                        puts("failed");
                                    }
                                    break;
                                }
                                else // In the group
                                {
                                    for (j = 3; j < strlen(buffer); j++) // gets the message sent
                                    {
                                        //strcat(broadcast_message, buffer[j]);
                                        strncpy(broadcast_message,buffer+3, (strlen(buffer)));
                                        puts("--");
                                        puts(broadcast_message);
                                        puts("--");
                                    }

                                    for (j = 0; j < max_clients; j++) // send it to all clients
                                    {
                                        if (clients[j].port_no != 0 && clients[i].socket != clients[j].socket)
                                        {
                                            if(send(clients[j].socket, buffer , strlen(buffer), 0 ) != strlen(buffer))
                                            {
                                                puts("failed");
                                            }   
                                        }
                                    }
                                }
                            }
                            else if (buffer[2] == '2')
                            {
                                if (clients[i].groups[1] == 0) // Not in the group
                                {
                                    if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                                    {
                                        puts("failed");
                                    }
                                    break;
                                }
                                else // In the group
                                {
                                    for (j = 3; j < strlen(buffer); j++) // gets the message sent
                                    {
                                        broadcast_message[index] = buffer[j];
                                        puts("--");
                                        puts(broadcast_message);
                                        puts("--");
                                    }

                                    for (j = 0; j < max_clients; j++) // send it to all clients
                                    {
                                        if (!clients[j].port_no || clients[i].socket != clients[i].socket)
                                        {
                                            if(send(clients[j].socket, broadcast_message , strlen(broadcast_message), 0 ) != strlen(broadcast_message))
                                            {
                                                puts("failed");
                                            }   
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            char *group_message;
                            group_message = "Lul what did you enter \n";

                            if(send(sd , group_message , strlen(group_message), 0 ) != strlen(group_message))
                            {
                                puts("failed");
                            }
                        }
                    }


                    //int test = (strcmp(buffer,"hello"));
                    if(send(sd , buffer , strlen(buffer), 0 ) != strlen(buffer)){
                    	puts("failed");
                    }
                    puts(buffer);
                }
            }
        }
    }
      
    return 0;
}