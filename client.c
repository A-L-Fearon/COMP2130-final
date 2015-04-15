
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
#define PEER_PORT   "50000"
#define MAX_CLIENTS 30
#define GRP_WORK    2
#define GRP_FUN     3

// 1 - main menu
// 2 - 

struct client {
    char *alias;
    int groups[2];
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


    struct sockaddr_in address; // used to creat peer to peer
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUFFER_SIZE];
    char recv[BUFFER_SIZE];
    char data[BUFFER_SIZE];

    char bytes_recv[BUFFER_SIZE];

    int opt = TRUE, master_socket , addrlen , new_socket, activity, i , valread , sd, max_sd;
    int sockfd, portno, n, initial = 1, rc, peer_socket;

    if ((server = gethostbyname("localhost")) == NULL)
    {   
        fprintf(stderr, "Error, no host\n");
        exit(0);
    }

    sender.listening = 0; // currently this user is not activelty listening
    sender.alias = "user";
    sender.portno = 0;
    sender.groups[0] = 0;
    sender.groups[1] = 0;

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

    // // delete this if anything
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
    printf("Enter an alias: ");

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
    puts(recv);
    
    

    // clear the socket set
    FD_ZERO(&readfds);

    // add read socket to set
    FD_SET(0, &readfds);
    FD_SET(sender.sockfd, &readfds);


    bzero(buffer, BUFFER_SIZE); // resets buffer          
    bzero(data, BUFFER_SIZE); // resets buffer

    printf("Main Menu Options\n");
    printf("\tCommand\t Option\n");
    puts("\t##\tGROUPS");        
    puts("\t@@\tPRIVATE");        
    

    while (TRUE)
    {
        // printf("user@user$ ");
        // fflush(stdout);

    // clear the socket set
        FD_ZERO(&readfds);

        // add read socket to set
        FD_SET(sender.sockfd, &readfds);
        FD_SET(0, &readfds);
        
        bzero(buffer, BUFFER_SIZE); // resets buffer          
        bzero(data, BUFFER_SIZE); // resets buffer

        // printf("Enter a command: ");


        activity = select( sender.sockfd + 1, &readfds , NULL , NULL , NULL);

        ///printf("%d \n", activity);

        if (select(sender.sockfd + 1, &readfds, NULL, NULL, NULL) < 0) 
        {
            printf("select error");
            // continue;
        }

        if (FD_ISSET(sender.sockfd, &readfds)) // receiving incomming data
        {
            if (read(sender.sockfd, buffer, (BUFFER_SIZE - 1)) < 0)
            {
                printf("Errror");
            }
            if (buffer[0] == '@') // someone is initiating a connection
            {
                printf("Some is trying to connect with you, enter '@y' to accept\n");
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
                if (buffer[1] == '#')
                {
                    printf("BROADCAST TO GROUPS");
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
                else if (buffer[1] == '+')
                {
                    printf("Attempting to create session....\n");

                    if ((peer_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
                    {
                        puts("[ERROR] Failed to create socker\n");
                        continue;
                    }

                    // creates tcp connection
                    address.sin_family = AF_INET;
                    address.serv_addr.s_addr = INADDR_ANY;
                    address.sin_port = atoi(PEER_PORT);

                    // bind socket to local host, port 50000
                    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
                    {
                        perror("bind failed");
                        exit(EXIT_FAILURE);
                    }

                    strcat(buffer, "|"); //
                    // strcat(buffer, portno) // attach your port number


                    write(sender.sockfd, buffer, strlen(buffer));
                }
                else
                {
                    write(sender.sockfd, buffer, strlen(buffer));
                }
            }
            else
            {
                write(sender.sockfd, buffer, strlen(buffer));
            }

            // if (strncmp(buffer, "@", 1) == 0) // peer to peer calls
            // {
            //     if (strlen(buffer) == 3) // REQUESTS THE  list of avaliable peers
            //     {
            //         write(sender.sockfd, buffer, strlen(buffer));
            //     }

            // }
            // else
            // {
            //     write(sender.sockfd, buffer, strlen(buffer));
            // }
        }

        // if (FD_ISSET(0, &readfds)) // handles user input
        // {
        //     bzero(buffer, BUFFER_SIZE); // resets buffer 

        //     fgets(buffer, (BUFFER_SIZE - 1), stdin);

        //     int s = strlen(buffer);
        //     // printf("CLient %s length(%d)\n", buffer, s);
        //     if (strncmp(buffer, "#", 1) == 0) // group calls
        //     {
        //         if (strlen(buffer) == 1)
        //         {
        //             printf("BROADCAST TO GROUPS");


        //         }

        //     }

        //     if (strncmp(buffer, "@@", 1) == 0) // peer to peer calls
        //     {
        //         if (strlen(buffer) == 3) // REQUESTS THE  list of avaliable peers
        //         {
        //             write(sender.sockfd, buffer, strlen(buffer));
        //         }

        //     }
        //     else
        //     {
        //         write(sender.sockfd, buffer, strlen(buffer));
        //     }
        //     continue;

        //     // if (write(sender.sockfd, buffer, strlen(buffer)) < 0)
        //     // {
        //     //     fprintf(stderr, "Error creating alias\n" );
        //     //     exit(0);
        //     // }

        //     // printf("%s\n", buffer);

        // }


    } // end of connections with client

    close(sender.sockfd);
}

//     while(TRUE)
//     {   
        
        
//         activity = select(sender.sockfd, &readfds, NULL, NULL, NULL);

//         if ((activity < 0) && (errno!=EINTR))
//         {
//             printf("select error");
//         }

//         if (FD_ISSET(sender.sockfd, &readfds)) // in comming data
//         {
//             printf("%d", sender.sockfd);

//             if (read(sender.sockfd, buffer, (BUFFER_SIZE - 1)) < 0)
//             {
//                 fprintf(stderr, "Error reading from socket");
//             }
            
   
//             // bzero(buffer, BUFFER_SIZE); // resets buffer
//         }
//         else // send data
//         {
//             puts("a");
//             // char data[BUFFER_SIZE];
//             // puts("here");
//             // switch(sender.write)
//             // {
//             //     case 0:
//             //         bzero(buffer, BUFFER_SIZE); // resets buffer
//             //         fgets(buffer, (BUFFER_SIZE - 1), stdin); // captures user input

//             //         data[0] = '#';
//             //         data[1] = '0'; 
                    
//             //         strcat(data, buffer);

//             //         if (write(sender.sockfd, data, strlen(data)) < 0)
//             //         {
//             //             fprintf(stderr, "Error writing to socket\n");
//             //             exit(0);       
//             //         }
//             //         break;
//             //     case 1:

//             //         break;
//             //     case 2:

//             //         break;
//             //     case 3:

//             //         break;
//             // }
//         }

//         if (FD_ISSET(0, &readfds)) // stdin
//         {
//             char data[BUFFER_SIZE];
//             bzero(buffer, BUFFER_SIZE); // resets buffer

//             switch(sender.write)
//             {
//                 case 0:
//                     fgets(buffer, (BUFFER_SIZE - 1), stdin); // captures user input
                    
//                     data[0] = '#';
//                     data[1] = '0'; 
                    
//                     strcat(data, buffer);

//                     if (write(sender.sockfd, data, strlen(data)) < 0)
//                     {
//                         fprintf(stderr, "Error writing to socket\n");
//                         exit(0);       
//                     }
//                     break;
//                 case 1:
//                     printf("fefefegeg\n");
//                     break;

//                 case 2:
//                     printf("appkpommim\n");
//                     break;
//             }
//         }  
//         else
//         {
//             puts("b");
//         }


//             // if (0 == strncmp(buffer, "Finish", 6))
//             // {
//             //     printf("\nAlert! Communication with server terminated...\n");
//             //     break;
//             // }
//     }
// }

// void process_listen(port)
// {
// }