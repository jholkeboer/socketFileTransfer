/*
Jack Holkeboer
Oregon State University 
CS372
Project 2
holkeboj@onid.oregonstate.edu
ftserver.cpp
Server socket based on: http://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
Client data socket based Beej's Guide: http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#simpleclient
Directory reading based on UNIX man pages: http://www.manpagez.com/man/3/opendir/
*/

#include <iostream>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nReceived SIGINT, terminating.\n");
        exit(1);
    }
    if (sig == SIGKILL) {
        printf("\nReceived SIGKILL, terminating.\n");
        exit(1);
    }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int port, initial_sock, connected_sock, data_sock, backlog = 5, rv,
        stop = 0, reuse = 1, first = 1, i = 0, j = 0, data_port_number, bytecount;
    struct dirent * dp;
    DIR * directory;
    char buffer[1024];
    char message[1024];
    struct addrinfo hints, *servinfo, *p; 
    struct sockaddr_in server_address, client_address, data_server_address;
    //struct sockaddr data_server_addr;
    struct hostent *data_server;
    socklen_t client_length;
    char char_handle[12];
    char quit[6] = "\\quit";
    char list[3] = "-l";
    char get[3] = "-g";
    char newline = '\n';
    char client_host[1024];
    char data_port[1024];
    char command[3];
    char filename[1024];
    char ch;    // used to clear buffer
    
    // handle sigint and sigkill
    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);
    
    // get port number from argument
    if (argc != 2) {
        printf("Please provide a port number as an argument.\n");
        exit(1);
    } else {
        port = atoi(argv[1]);
        if ((port < 20000) || (port > 65535)) {
            printf("Please enter a port number between 20000 and 65535.\n");
            exit(1);
        }
        printf("Welcome to the file transfer server.\n");
    }

    // start server
    while(1) {
        // reset connection close flag
        stop = 0;

        if (first) {
            // Create Socket
            initial_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (initial_sock < 0) {
                printf("Could not open socket. Exiting.\n");
                exit(1);
            }

            // clear server address
            bzero((char*) &server_address, sizeof(server_address));

            // Specify address type
            server_address.sin_family = AF_INET;
            
            // Whatever IP this program is running on will be assigned
            server_address.sin_addr.s_addr = INADDR_ANY;
            
            // Attach port number, convert to network btye order
            server_address.sin_port = htons(port);

            // Make socket reusable
            setsockopt(initial_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

            printf("Waiting on port %d ... \n",port);

            // Bind to socket.  This socket will accept initial conenctions from clients
            if (bind(initial_sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
                printf("Could not bind to socket. Exiting.\n");
            }

            // Listen for connections
            listen(initial_sock, backlog);

        } else {
            printf("Waiting for a new client to connect...\n");
        }

        // accept connection
        connected_sock = accept(initial_sock, (struct sockaddr *) &client_address, &client_length);
        if (connected_sock < 0) {
            printf("Connection Failed. Exiting.\n");
            exit(1);        
        }
        
        // get client hostname
        getpeername(connected_sock, (struct sockaddr*) &client_address, &client_length);
        getnameinfo((struct sockaddr*) &client_address, sizeof(client_address), client_host, sizeof(client_host), NULL, 0, 0);
        
        printf("Accepted connection from client.\n");
        printf("Client hostname is %s.\n", client_host);
        data_server = gethostbyname(client_host);
        while (!stop) {
            // clear buffer
            bzero(buffer,1024);
            printf("[waiting for response...]\n");
            
            // read from socket
            if (read(connected_sock,buffer,1024) < 0) {
                printf("Could not read from socket.\n");
            } else {
                //read command (first two letters)
                i = 0;
                for (i = 0; i < 2; i++) {
                    command[i] = buffer[i];
                }
                if ((strcmp(command,list) != 0) && (strcmp(command,get) != 0)) {
                    bzero(buffer, 1024);
                    strcpy(buffer, "ERROR\n");
                    if (write(connected_sock, buffer, 1024) < 0) {
                        printf("Could not send to socket.\n");
                    }                    
                } else {
                    // get data port
                    j = 0;
                    while (buffer[i] != '\n') {
                        data_port[j] = buffer[i];
                        i++;
                        j++;
                    }   
                    data_port[j] = '\0';
                    data_port_number = atoi(data_port);
                    printf("Data port %d\n", data_port_number);
                    
                    // make data connection to client
                    memset(&hints, 0, sizeof hints);
                    hints.ai_family = AF_INET;
                    hints.ai_socktype = SOCK_STREAM;
                    
                    if ((rv = getaddrinfo(client_host, data_port, &hints, &servinfo)) != 0) {
                        printf("Error resolving client address for data connection.\n");
                        exit(1);
                    }
                    // look for valid connection
                    for (p = servinfo; p != NULL; p = p->ai_next) {
                        if ((data_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                            close(data_sock);
                            printf("Data socket invalid.\n");
                            continue;
                        }
                        
                        if (connect(data_sock, p->ai_addr, p->ai_addrlen) == -1) {
                            close(data_sock);
                            printf("Coudln't connect to data socket.\n");
                            continue;
                        }
                        break;
                    }
                    
                    if (p == NULL) {
                        close(data_sock);
                        printf("Could not find data socket.\n");
                        exit(1);
                    }
                    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), client_host, sizeof(client_host));
                    printf("Connecting to client on %s", client_host);
                    freeaddrinfo(servinfo);
                    
                    if (strcmp(command,list) == 0) {
                        bzero(buffer,1024);
                        
                        // send directory contents
                        directory = opendir(".");
                        i = 0;
                        while ((dp = readdir(directory)) != NULL) {
                            i = i + dp->d_namlen;
                            if (i >= 1024) {
                                if (write(data_sock, (struct sockaddr *) &data_server_address, sizeof(data_server_address)) < 0) {
                                    printf("Error writing to socket.\n");
                                }
                                bzero(buffer, 1024);
                                i = 0;
                            }
                            strcpy(buffer + (i * sizeof(int)), dp->d_name);
                            strcpy(buffer + (i * sizeof(int)) + (dp->d_namlen * sizeof(int)), &newline);
                        }
                        close(data_sock);                        
                    } else if (strcmp(command,get) == 0) {
                        close(data_sock);
                    }
                }
                // if (strcmp(command,list) == 0) {
                //     printf("Received request for file list.\n");

                //     // make data connection with client
                //     // data_sock = socket(AF_INET, SOCK_STREAM, 0);
                //     // printf("Initialized data_sock.\n");
                //     // if (data_sock < 0) {
                //     //     printf("Could not open data connection to client.\n");
                //     // }
                //     // bzero((char *) &data_server, sizeof(struct hostent));
                //     // bzero((char *) &data_server_address, sizeof(data_server_address));
                //     // printf("Reset data server address.\n");
                //     // data_server_address.sin_family = AF_INET;
                //     // printf("Assigned data sin_family.\n");
                //     // bcopy((char *) data_server->h_addr, (char *) &data_server_address.sin_addr.s_addr, data_server->h_length);
                //     // printf("Copied data server address.\n");
                //     // data_server_address.sin_port = htons(data_port_number);
                //     // if (connect(data_sock,(struct sockaddr *) &data_server_address,sizeof(data_server_address)) < 0) {
                //     //     printf("Could not make data connection to client.\n");
                //     // }
                    

                // } else if (strcmp(command,get) == 0) {
                    
                // }
            } 
                // check if \quit was received from client
                // if (strcmp(buffer, quit) == 0) {
                //     printf("Client closed the connection.\n");
                //     shutdown(connected_sock,2);
                //     close(connected_sock);
                    
                //     // set control flags
                //     stop = 1;
                //     first = 0;
                // } else {
                //     // Print received message
                //     printf("%s\n", buffer);
                    

                //     // send message
                //     printf("%s> ", char_handle);
                //     bzero(buffer,512);
                //     bzero(message,500);

                //     // get user message input
                //     std::cin.getline(message, 500);
                    
                //     // copy handle and message to buffer
                //     i = 0;
                //     while (char_handle[i] != '\0') {
                //         buffer[i] = char_handle[i];
                //         i++;
                //     }
                //     buffer[i] = '>';
                //     i++;
                //     buffer[i] = ' ';
                //     i++;
                //     strcpy(buffer + (i * sizeof(int)), message);
                    
                //     // check for quit signal
                //     if (strcmp(message, quit) == 0) {
                //         printf("Closing the connection.\n");
                //         write(connected_sock,quit,6);
                //         shutdown(connected_sock,2);
                //         close(connected_sock);
                //         stop = 1;
                //         first = 0;
                //     } else{
                //         // write message to socket
                //         if ((write(connected_sock,buffer, 512) < 0)) {
                //             printf("Could not send to socket.\n");
                //         }
                //     }
                // }
        }
    }

    return 0;
}