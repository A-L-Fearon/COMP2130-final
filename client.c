
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <unistd.h>   //close
#include <arpa/inet.h>    //close

#define BUFFER_SIZE 1024
#define TRUE        1
#define FALSE       0
#define PORT        60000
#define MAX_CLIENTS 30
#define GRP_WORK    2
#define GRP_FUN     3

struct client {
    char *alias;
    int group;
    int portno;
    int listening;
    int sockfd;
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

    struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    int pid; // process id
    char buffer[BUFFER_SIZE];


    int opt = TRUE, master_socket , addrlen , new_socket, activity, i , valread , sd, max_sd;
    int sockfd, portno, n, initial = 1, rc;

    if ((server = gethostbyname("localhost")) == NULL)
    {   
        fprintf(stderr, "Error, no host\n");
        exit(0);
    }

    

    sender.listening = 0; // currently this user is not activelty listening
    sender.alias = "user";
    sender.portno = 0;
    sender.group = 0;

    portno = atoi("60000");

    if ((sender.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);

    if (connect(sender.sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
    {
        fprintf(stderr, "Error connecting\n");
        exit(0);
    }

    // // delete this if anything
    // if( setsockopt(sender.sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }


    system("clear");

    printf("*************************************************\n");
    printf("\t\tWelcome to C Chat\t\t\n");
    printf("*************************************************\n\n");
    printf("Enter an alias\n");
    

    // int err;
    // pthread_t thread; 

    // err = pthread_create(&thread, NULL, &list_response, NULL);

    // pid

    while(TRUE)
    {   
        // clear the socket set
        FD_ZERO(&readfds);

        // add read socket to set
        FD_SET(sender.sockfd, &readfds);
        FD_SET(0, &readfds);
        
        activity = select(sender.sockfd, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        if (FD_ISSET(sender.sockfd, &readfds)) // in comming data
        {
            printf("%d", sender.sockfd);

            if (read(sender.sockfd, buffer, (BUFFER_SIZE - 1)) < 0)
            {
                fprintf(stderr, "Error reading from socket");
                
            }
            puts(buffer);

            if (sender.portno == 0)
            {
                sender.portno = atoi(buffer);
                printf("%d\n", sender.portno);
                printf("Enter an alias: ");
            }
            else if (strcmp(sender.alias, "user") == 0)
            {
                sender.alias = buffer;

                printf("%s\n", buffer);
            }
            puts(buffer);
            // bzero(buffer, BUFFER_SIZE); // resets buffer
        }
        else // send data
        {
            // bzero(buffer, BUFFER_SIZE); // resets buffer

            // printf("Enter data\n");
            // fgets(buffer, (BUFFER_SIZE - 1), stdin); // captures user input

            // n = write(sender.sockfd, buffer, strlen(buffer));

            // if (n < 0)
            // {
            // }

            // printf("%s\n", buffer);
        }

        if (FD_ISSET(0, &readfds)) // stdin
        {
            printf("Enter data: ");
            fgets(buffer, (BUFFER_SIZE - 1), stdin);

            if (write(sender.sockfd, buffer, strlen(buffer)) < 0)
            {
                fprintf(stderr, "Error writing to socket\n");
                exit(0);
            }
        }  
        else
        {
            printf("b");
        }


            // if (0 == strncmp(buffer, "Finish", 6))
            // {
            //     printf("\nAlert! Communication with server terminated...\n");
            //     break;
            // }


        // if (sender.listening == 0) // user is not listening 
        // {
        //     int m_portno = atoi(buffer);
        //     // pthread_create(&listen_thread, NULL, listening, (void *)m_portno);
        //     // sender.listening = 1; // now listening

        //     pid = fork();

        //     if (pid == 0) // child
        //     {
        //         process_listen(m_portno);
        //     }
        //     else
        //     {

        //     }

        // }

    }
}

void process_listen(port)
{
}