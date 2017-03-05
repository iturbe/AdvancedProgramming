/*
    Program for a simple chat client
    The server address is provided as arguments to the program

    Gilberto Echeverria
    gilecheverria@yahoo.com
    26/02/2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>

#define SERVICE_PORT 8642
#define BUFFER_SIZE 1024
#define NAME_SIZE 20

void usage(char * program);
void connectToServer(char * address, char * port);
void communicationLoop(int connection_fd);

int main(int argc, char * argv[])
{
    printf("\n=== CLIENT PROGRAM ===\n");

    if (argc != 3)
        usage(argv[0]);

    connectToServer(argv[1], argv[2]);

    return 0;
}

// Show the user how to run this program
void usage(char * program)
{
    printf("Usage:\n%s {server_address} {port_number}\n", program);
    exit(EXIT_FAILURE);
}

// Establish a connection with the server indicated by the parameters
void connectToServer(char * address, char * port)
{
    struct addrinfo hints;
    struct addrinfo * server_info;
    int connection_fd;

    // Prepare the information to determine the local address
    // Clear all fields
    bzero(&hints, sizeof hints);
    // Use internet sockets with IPv4
    hints.ai_family = AF_INET;
    // Use connected sockets
    hints.ai_socktype = SOCK_STREAM;
    // Determine the ip of this machine automatically
    hints.ai_flags = AI_PASSIVE;

    ///// GETADDRINFO
    // Get the actual address of this computer
    if ( getaddrinfo(address, port, &hints, &server_info) == -1 )
    {
        perror("ERROR: getaddrinfo");
        exit(EXIT_FAILURE);
    }

    ///// SOCKET
    // Use the information obtained by getaddrinfo
    connection_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if ( connection_fd == -1 )
    {
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }

    ///// CONNECT
    // Connect with the desired port
    if ( connect(connection_fd, server_info->ai_addr, server_info->ai_addrlen) == -1 )
    {
        perror("ERROR: connect");
        exit(EXIT_FAILURE);
    }

    // Release the memory of the list of addrinfo
    freeaddrinfo(server_info);

    // Establish the communication
    communicationLoop(connection_fd);

    // Close the socket
    close(connection_fd);
}

// Do the actual receiving and sending of data
void communicationLoop(int connection_fd)
{
    char buffer[BUFFER_SIZE];
    int message_counter = 0;
    char hand[BUFFER_SIZE];
    int sum = 0;
    char message[BUFFER_SIZE];

    while (1)
    {
        // WRITE
        if (message_counter == 0) { //initial message
          printf("You are now seated at the table, let the dealer know your name.\n");
          printf("Name: ");

        } else if (message_counter == 1) { //got cards, send bet
          sprintf(hand, "%s", buffer);
          //strtok(hand, "\n"); //clean up input
          printf("How much would you like to bet?\nAmount: ");

        } else if (message_counter == 2) { //initial hit or stand
          printf("DEALER: %s\n", buffer);
          printf("Would you like to hit or stand?\n");
          printf("Your current hand is: %s\n", hand);
          printf("Choice (h/s): ");

        } else { //hit or stand loop
          sscanf(buffer, "%s %s", &hand, &message); //get latest hand and message
          printf("Your current hand is: %s\n", hand);
          if (strlen(message) > 0) { //print dealer message, if one was sent
            printf("DEALER: %s\n", message);
          }
          printf("Would you like to hit or stand?\nChoice (h/s): ");

        }

        // get user input
        fgets(buffer, BUFFER_SIZE, stdin);

        // check if the connection has ended
        if (strlen(buffer) == 1)
        {
            printf("Finishing the connection\n");
            break;
        }
        // SEND
        if ( send(connection_fd, buffer, strlen(buffer)+1, 0) == -1 )
        {
            perror("ERROR: send");
            exit(EXIT_FAILURE);
        }

        message_counter++;

        //LISTEN
        // Clear the buffer
        bzero(buffer, BUFFER_SIZE);
        // Read the message from the server
        if ( recv(connection_fd, buffer, BUFFER_SIZE, 0) == -1 )
        {
            perror("ERROR: recv");
            exit(EXIT_FAILURE);
        }
        //printf("The server replied with: %s\n", buffer);
        //printf("DEALER: %s\n", buffer);
        strtok(buffer, "\n"); //clean up input

    }
}
