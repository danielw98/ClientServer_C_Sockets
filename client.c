// aici facem un client simplu de la zero
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8090


int main()
{
    int connection_socket;
    struct sockaddr_in server_address;
    char request[1024] = "Hello from client!";
    char response[1024];

    connection_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(connection_socket == -1)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if(inet_addr(SERVER_IP) == INADDR_NONE)
    {
        perror("Invalid IP");
        exit(EXIT_FAILURE);
    }
    server_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    // facem conexiunea la server
    if(connect(connection_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1)
    {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }
    // aici incepe seisunea
    int request_length = strlen(request) + 1;
    write(connection_socket, &request_length, sizeof(int));
    write(connection_socket, request, request_length);

    printf("Am trimis mesajul catre server: %s\n", request);

    int response_length;
    read(connection_socket, &response_length, sizeof(int));
    read(connection_socket, &response, response_length);

    printf("Raspunsul de la server: %s\n", response);
    // aici se termina sesiunea
    close(connection_socket);

    return 0;
}