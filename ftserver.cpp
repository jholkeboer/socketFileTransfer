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
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
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

// get sockaddr function, from Beej's guide
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int port, initial_sock, connected_sock, data_sock, backlog = 5, rv,
        stop = 0, reuse = 1, first = 1, i = 0, j = 0, data_port_number, bytecount = 0;
    struct dirent * dp;
    DIR * directory;
    char buffer[1024];
    struct addrinfo hints, *servinfo, *p; 
    struct sockaddr_in server_address, client_address, data_server_address;
    //struct sockaddr data_server_addr;
    struct hostent *data_server;
    struct stat file_info;
    socklen_t client_length;
    char list[3] = "-l";
    char get[3] = "-g";
    char client_host[1024];
    char data_port[1024];
    char command[3];
    char filename[1000];
    char filedir[1024];
    FILE *fp;
    
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
            first = 0;
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
        bzero(buffer,1024);
        printf("[waiting for response...]\n");
        
        // read from socket
        if (read(connected_sock,buffer,1024) < 0) {
            printf("Could not read from socket.\n");
        } else {
            //read command (first two letters)
            printf("%s\n",buffer);
            i = 0;
            for (i = 0; i < 2; i++) {
                command[i] = buffer[i];
            }
            command[2] = '\0';
            i++;
            if ((strcmp(command,list) != 0) && (strcmp(command,get) != 0)) {
                // Invalid command, send error message to client
                bzero(buffer, 1024);
                strcpy(buffer, "ERROR\0");
                printf("Invalid command: %s\n", command);
                if (write(connected_sock, buffer, 1024) < 0) {
                    printf("Could not send to socket.\n");
                }
                close(connected_sock);                  
            } else {
                // get data port
                j = 0;
                while ((buffer[i] != '\n') && (buffer[i] != '\t')) {
                    data_port[j] = buffer[i];
                    i++;
                    j++;
                }   
                data_port[j] = '\0';
                data_port_number = atoi(data_port);
                printf("Data port %d\n", data_port_number);
                
                // get filename if provided
                if (buffer[i] == '\t') {
                    bzero(filename, 1000);
                    i++;
                    j = 0;
                    while (buffer[i] != '\n') {
                        filename[j] = buffer[i];
                        i++;
                        j++;
                    }
                    filename[j] = '\0';
                }
                
                // Send "OK" to client
                bzero(buffer, 1024);
                strcpy(buffer, "OK\0");
                if (write(connected_sock, buffer, 1024) < 0) {
                    printf("Could not send to socket.\n");
                }
                sleep(1);
                
                // make data connection to client
                memset(&hints, 0, sizeof hints);
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                
                if ((rv = getaddrinfo(client_host, data_port, &hints, &servinfo)) != 0) {
                    printf("Error resolving client address for data connection.\n");
                    exit(1);
                }

                // look for datavalid connection (based on Beej's guide)
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
                
                // check for null socket
                if (p == NULL) {
                    printf("Could not find data socket.\n");
                    exit(1);
                }
                inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), client_host, sizeof(client_host));
                printf("Connecting to client on %s port %d\n", client_host, data_port_number);
                freeaddrinfo(servinfo);

                // react to commands
                if (strcmp(command,list) == 0) {
                    // RECEIVED LIST COMMAND
                    bzero(buffer,1024);
                    printf("Sending directory contents.\n");

                    // send directory contents
                    directory = opendir(".");
                    i = 0;
                    while (((dp = readdir(directory)) != NULL) && (i < 1024)) {
                        // loop through directory
                        // continue if it's not a file
                        stat(dp->d_name, &file_info);
                        if (S_ISDIR(file_info.st_mode)) {
                            continue;
                        }
                        i = i + dp->d_reclen;
                        if (i >= 1024) {
                            break;
                        }                        

                        printf("%s\n", dp->d_name);
                        write(data_sock, dp->d_name, dp->d_reclen);
                    }
                    close(data_sock);
                } else if (strcmp(command,get) == 0) {
                    // file exists.  send the file
                    bzero(buffer,1024);
                    
                    // build path
                    bzero(filedir, 1024);
                    strcpy(filedir, "./");
                    strcpy(filedir + (2 * sizeof(char)), filename);
                    fp = fopen(filedir, "r");
                    
                    // check for null file
                    if (fp == NULL) {
                        // file is null, send error message
                        printf("File pointer is null for filename %s\n",filename);
                        strcpy(buffer, "ERROR\0");
                        if (write(data_sock, buffer, 1024) < 0) {
                            printf("Could not send error msg to socket.\n");
                        }
                    } else {
                        // read from file
                        printf("Sending file %s...\n", filename);
                        fseek(fp, 0, SEEK_SET);
                        bytecount = fread(buffer, sizeof(char), 1023, fp);
                        while ((bytecount < 1024) && (bytecount > 0)) {
                            buffer[bytecount] = '\0';
                            if (write(data_sock, buffer, 1024) < 0) {
                                printf("Could not send file chunk to socket.\n");
                            }
                            bytecount = fread(buffer, sizeof(char), 1023, fp);
                        }
                        fclose(fp);
                    }
                    close(data_sock);
                } else {
                    printf("Didn't parse command correctly.\n");
                    signal_handler(SIGKILL);
                }
            }
        }
    }

    return 0;
}