
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>   //close
#include <arpa/inet.h>    //close

#define BUFFER_SIZE 1024
#define TRUE        1
#define FALSE       0
#define SERVER_PORT "60000"
#define MAX_CLIENTS 30
#define GRP_WORK    2
#define GRP_FUN     3

int randint(int max);
// 1 - main menu
// 2 - 

struct client {
    char alias[30];
    int groups[2];
    int portno;
    int listening;
    int sockfd;
    int peer;
};

struct client sender; // this client
struct client receiver; // delever the private message to   

void ok()
{
    exit(1);
}

void main(int argc, char *argv[])
{
    fd_set readfds;


    struct sockaddr_in address, address2; // used to creat peer to peer
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    struct hostent *server;
    struct hostent *peer;

    char buffer[BUFFER_SIZE];
    char recv[BUFFER_SIZE];
    char data[BUFFER_SIZE];

    char bytes_recv[BUFFER_SIZE];

    int opt = TRUE, addrlen , new_socket, activity, i , valread , sd, max_sd;
    int peer_sockfd, portno, n, initial = 1, rc;
    int peer_socket = -1; // file descriptor for peer to peer connection

    if ((server = gethostbyname("localhost")) == NULL)
    {   
        fprintf(stderr, "Error, no host\n");
        exit(0);
    }

    sender.listening = 0; // currently this user is not activelty listening
    sender.portno = -1;
    sender.groups[0] = 0;
    sender.groups[1] = 0;
    sender.peer = -1; // peer's port number

    portno = atoi(SERVER_PORT);

    if ((sender.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(0);
    }
    printf("%d\n\n", sender.sockfd);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);

    if (connect(sender.sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        fprintf(stderr, "Error connecting\n");
        exit(0);
    }

    if( setsockopt(sender.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    bzero(buffer, BUFFER_SIZE); // resets buffer          
    bzero(data, BUFFER_SIZE); // resets buffer

    system("clear");

    printf("*************************************************\n");
    printf("\t\tWelcome to C Chat\t\t\n");
    printf("*************************************************\n");
    printf("Please Enter An Alias (username): ");

    fgets(buffer, (BUFFER_SIZE - 1), stdin);

    if (write(sender.sockfd, buffer, strlen(buffer)) < 0)
    {
        fprintf(stderr, "Error creating alias\n" );
        exit(0);
    }

    if (read(sender.sockfd, recv, (BUFFER_SIZE - 1)) < 0)
    {
        printf("Error reading from socket\n");
        exit(0);
    }

    sender.alias = buffer;
    //puts(recv);
    printf("\n Welcome %s \n ", recv);

    //strtok(buffer, "\n");

    
    strcpy(sender.alias, buffer);
    puts(recv);

    // clear the socket set
    FD_ZERO(&readfds);

    // add read socket to set
    FD_SET(0, &readfds);
    FD_SET(sender.sockfd, &readfds);


    bzero(buffer, BUFFER_SIZE); // resets buffer          
    bzero(data, BUFFER_SIZE); // resets buffer

    printf("Main Menu Options\n");
    printf("Please follow comands below (Refer to README for more information))\n ");
    printf("\tCommand\t Option\n");
    puts("\t##\t For Available Groups And Group Commands");        
    puts("\t@@\tFor A List Of Connected Users and Private Msg Commands");        
    

    while (TRUE)
    {
        int max_fd = sender.sockfd;

        // printf("user@user$ ");
        // fflush(stdout);

    // clear the socket set
        FD_ZERO(&readfds);

        // add read socket to set
        FD_SET(sender.sockfd, &readfds);
        FD_SET(0, &readfds);

        if (peer_socket != -1)
        {
            FD_SET(peer_socket, &readfds);

            if (peer_socket > sender.sockfd)
                max_fd = peer_socket;
        }
        
        bzero(buffer, BUFFER_SIZE); // resets buffer          
        bzero(data, BUFFER_SIZE); // resets buffer

        // printf("Enter a command: ");

        //printf(">");
        activity = select( max_fd + 1, &readfds , NULL , NULL , NULL);

        if (select(sender.sockfd + 1, &readfds, NULL, NULL, NULL) < 0) 
        {
            printf("select error");
            // continue;
        }

        if (peer_socket != -1)
        {
            if (FD_ISSET(peer_socket, &readfds))
            {
                if (read(peer_socket, buffer, (BUFFER_SIZE - 1)) < 0)
                {
                    printf("Error read client message\n");
                    continue;
                }
                puts(buffer);
            }
        }
        
        if (FD_ISSET(sender.sockfd, &readfds)) // receiving incomming data
        {
            if (read(sender.sockfd, buffer, (BUFFER_SIZE - 1)) < 0)
            {
                printf("Errror");
                continue;
            }
            if (buffer[0] == '@') // someone is initiating a connection
            {
                if (buffer[1] == '+')
                {
                    char peer_port[BUFFER_SIZE], peer_name[BUFFER_SIZE];
                    int i, j = 3, count = 0, port_to_int; // begining of the port number

                    while (buffer[j] != '|')
                    {
                        printf("%c\n", buffer[j]);
                        peer_port[count] = buffer[j];
                        j++;
                        count++;
                    }

                    port_to_int = atoi(peer_port); // saves the port number
                    count = 0; // resets the index
                    j++; // j is in the index for the person's name

                    for (i = j; i < strlen(buffer); i++)
                    {
                        peer_name[count] = buffer[i];
                        count++;
                    }
                    strtok(peer_name, "\n");

                    // saves the alias
                    strcpy(sender.alias, peer_name); 

                    // saves the peer port number
                    sender.peer = port_to_int; 

                    printf("%s is trying to private chat with you on port %d\n", peer_name, port_to_int);

                    printf("Enter @y to accept or @n to decline when ready: ");
                }
            }
            puts(buffer);
        }
        else
        {
            bzero(buffer, BUFFER_SIZE); // resets buffer 

            fgets(buffer, (BUFFER_SIZE - 1), stdin);

            int s = strlen(buffer);
            // printf("CLient %s length(%d)\n", buffer, s);
            if (buffer[0] == '#') // group calls
            {
                if (sender.portno != -1) 
                {
                    puts("Broadcast options not available in p2p");
                    continue;
                }
                if (buffer[1] == '#')
                {
                    printf("GROUP OPTIONS \n");
                    printf("-------------- \n");
                }
                write(sender.sockfd, buffer, strlen(buffer));
            }
            else if (buffer[0] == '@')
            {
                if (buffer[1] == '@')
                {
                    printf("Requesting list of peers...\n");
                    write(sender.sockfd, buffer, strlen(buffer));
                }
                else if (buffer[1] == 'y')
                {
                    if (sender.portno != -1) // already in a p2p
                    {
                        puts("You are already in a p2p connection");
                        continue;   
                    }

                    strtok(buffer, "\n");

                    puts ("Creating p2p connection");

                    if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
                    {
                        puts("[ERROR] Failed to create socker\n");
                        continue;
                    }

                    // port #'s will range from 60000 ~ 700000
                    sender.portno = randint(1000) + 60000;

                    // creates tcp connection
                    address2.sin_family = AF_INET;
                    address2.sin_addr.s_addr = INADDR_ANY;
                    address2.sin_port = sender.portno;

                    // bind socket to local host, port
                    if (bind(peer_socket, (struct sockaddr *)&address2, sizeof(address2))<0) 
                    {
                        perror("bind failed");
                        exit(EXIT_FAILURE);
                    }

                    printf("Peer sock %d\nListening for peer on port %d\n", peer_socket, sender.portno);

                    if (listen(peer_socket, 2) < 0)
                    {
                        printf("Error listening\n");
                        exit(EXIT_FAILURE);
                    }

                    puts("Here");

                    char peer_port[10];

                    sprintf(peer_port, "%d", sender.portno);

                    addrlen = sizeof(address2);
                    puts("Now waitng for connections ...\n");

                    strcat(buffer, peer_port);

                    write(sender.sockfd, buffer, strlen(buffer)); // alerts server

                    /*
                    Connects creates socket fd to message client 
                    */

                    if ((peer_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        fprintf(stderr, "Error opening socket to peer\n" );
                        exit(0);
                    }

                    bzero((char *) &peer_addr, sizeof(peer_addr));

                    peer_addr.sin_family = AF_INET;

                    bcopy((char *)peer->h_addr, (char *)&peer_addr.sin_addr.s_addr, peer->h_length);

                    peer_addr.sin_port = htons(sender.peer);

                    if (connect(peer_sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
                    {
                        fprintf(stderr, "Error connecting\n");
                        exit(0);
                    }

                    if( setsockopt(sender.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
                    {
                        perror("setsockopt");
                        exit(EXIT_FAILURE);
                    }

                }
                else if (buffer[1] == 'n')
                {
                    if (sender.portno != -1)// already in a p2p
                    {
                        puts("You are already in a p2p connection");
                        continue;   
                    }
                }
                else if (buffer[1] == '+')
                {
                    if (sender.portno != -1) // already in a p2p
                    {
                        puts("You are already in a p2p connection");
                        continue;
                    }
                    strtok(buffer, "\n");

                    printf("Attempting to create session....\n");

                    if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
                    {
                        puts("[ERROR] Failed to create socker\n");
                        continue;
                    }

                    // port #'s will range from 60000 ~ 700000
                    sender.portno = randint(1000) + 60000;

                    // creates tcp connection
                    address.sin_family = AF_INET;
                    address.sin_addr.s_addr = INADDR_ANY;
                    address.sin_port = sender.portno;

                    //address.sin_port = sender.portno;

                    // bind socket to local host, port
                    if (bind(peer_socket, (struct sockaddr *)&address, sizeof(address))<0) 
                    {
                        perror("bind failed");
                        exit(EXIT_FAILURE);
                    }

                    printf("Peer sock %d\nListening for peer on port %d\n", peer_socket, sender.portno);

                    if (listen(peer_socket, 2) < 0)
                    {
                        printf("Error listening\n");
                        exit(EXIT_FAILURE);
                    }

                    puts("Here");

                    char peer_port[10];

                    sprintf(peer_port, "%d", sender.portno);

                    addrlen = sizeof(address);
                    puts("Now waitng for connections ...\n");

                    strcat(buffer, "|"); // add delimiter
                    strcat(buffer, peer_port); // attach your port number
                    strcat(buffer, "|"); // attach delimiter
                    puts(buffer);

                    write(sender.sockfd, buffer, strlen(buffer));
                }
                else
                {
                    if (sender.portno != -1) 
                        write(peer_sockfd, buffer, strlen(buffer)); // p2p
                    else 
                        write(sender.sockfd, buffer, strlen(buffer)); // client server
                }
            }
            else
            {
                if (sender.portno != -1) 
                    write(peer_sockfd, buffer, strlen(buffer)); // p2p
                else 
                    write(sender.sockfd, buffer, strlen(buffer)); // client server
            }
        }


    } // end of connections with client

    close(sender.sockfd);
}

int randint(int max)
{
    return rand() % 1000;
}
