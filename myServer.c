/*
    Program for a simple chat server
    Can only connect with one client at a time

    Gilberto Echeverria
    gilecheverria@yahoo.com
    23/02/2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// Include libraries for sockets
#include <netdb.h>
#include <arpa/inet.h>

#define SERVICE_PORT 8642
#define MAX_QUEUE 5
#define BUFFER_SIZE 1024

void usage(char * program);
void startServer(char * port);
void waitForConnections(int server_fd);
void communicationLoop(int connection_fd);

int main(int argc, char * argv[])
{
    printf("\n=== SERVER PROGRAM ===\n");

    if (argc != 2){
      usage(argv[0]);
    }

    printf("How many players will connect?\n# players: ");
    int players = 0;
    scanf("%d", &players);

    for (int a = 0; a < players; a++) { //fork the number of players

      //set up fork
      pid_t pid;
      pid = fork();

      if (pid > 0){ //parent
        printf("I am the parent, continuing loop...\n");
        continue;

      } else if (pid == 0){ //child
        printf("child #%d forked\n", a);

        int temp = atoi(argv[1]); //convert to int
        temp += a; //add a
        char converted[BUFFER_SIZE];
        sprintf(converted, "%d", temp); //convert back to char *

        printf("Starting server on port %d\n", temp);
        startServer(converted);

      } else { //fork failed
        perror("Unable to create a new process\n");
        exit(EXIT_FAILURE);
      }

    }


    //wait for all children to terminate
    printf("Parent waiting for children to terminate\n");
    for(int a=0; a < players; a++){
      printf("At least i'm in the loop\n");
      pid_t  pid;
      int status = 0;
      pid = wait(&status);
      printf("Process #%d done", a);
    }
    //wait(NULL);
    printf("All children have terminated, exiting...\n");

    return 0;
}

// Show the user how to run this program
void usage(char * program)
{
    printf("Usage:\n%s {port_number}\n", program);
    exit(EXIT_FAILURE);
}

// Initialize the server to be ready for connections
void startServer(char * port)
{
    struct addrinfo hints;
    struct addrinfo * server_info;
    int server_fd;

    // Prepare the information to determine the local address
    // Clear all fields
    bzero(&hints, sizeof hints);
    // Use internet sockets with IPv4
    hints.ai_family = AF_INET;
    // Use connected sockets
    hints.ai_socktype = SOCK_STREAM;
    // Determine the ip of this machine automatically
    //hints.ai_flags = AI_PASSIVE;
    hints.ai_flags = AI_NUMERICHOST;

    ///// GETADDRINFO
    // Get the actual address of this computer
    if ( getaddrinfo(NULL, port, &hints, &server_info) == -1 )
    {
        perror("ERROR: getaddrinfo");
        exit(EXIT_FAILURE);
    }

    ///// SOCKET
    // Use the information obtained by getaddrinfo
    server_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if ( server_fd == -1 )
    {
        perror("ERROR: socket");
        exit(EXIT_FAILURE);
    }

    ///// BIND
    // Connect with the desired port
    if ( bind(server_fd, server_info->ai_addr, server_info->ai_addrlen) == -1 )
    {
        perror("ERROR: bind");
        exit(EXIT_FAILURE);
    }

    // Release the memory of the list of addrinfo
    freeaddrinfo(server_info);

    ///// LISTEN
    // Prepare for connections
    if ( listen(server_fd, MAX_QUEUE) == -1 )
    {
        perror("ERROR: listen");
        exit(EXIT_FAILURE);
    }

    printf("Server ready and waiting!\n");
    // Stand by to receive connections from the clients
    waitForConnections(server_fd);

    // Close the socket
    close(server_fd);
}

// Stand by for connections by the clients
void waitForConnections(int server_fd)
{
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    char client_presentation[INET_ADDRSTRLEN];
    int connection_fd;

    // Loop to wait for client connections
    while (1)
    {
        ///// ACCEPT
        // Receive incomming connections
        // Get the size of the structure where the address of the client will be stored
        client_address_size = sizeof client_address;
        // Receive the data from the client and open a new socket connection
        connection_fd = accept(server_fd, (struct sockaddr *) &client_address, &client_address_size);
        if ( connection_fd == -1 )
        {
            perror("ERROR: accept");
            exit(EXIT_FAILURE);
        }

        // Identify the client
        // Get the ip address from the structure filled by accept
        inet_ntop(client_address.sin_family, &client_address.sin_addr, client_presentation, sizeof client_presentation);
        printf("Received connection from: %s : %d\n", client_presentation, client_address.sin_port);

        // Establish the communication
        communicationLoop(connection_fd);
    }
}

// Do the actual receiving and sending of data
void communicationLoop(int connection_fd)
{
    char buffer[BUFFER_SIZE];
    int message_counter = 0;
    int chars_read;

    while (1)
    {
        // Clear the buffer
        bzero(buffer, BUFFER_SIZE);

        ///// RECV
        // Read the request from the client
        chars_read = recv(connection_fd, buffer, BUFFER_SIZE, 0);
        // Error when reading
        if ( chars_read == -1 )
        {
            perror("ERROR: recv");
            exit(EXIT_FAILURE);
        }
        // Connection finished
        if ( chars_read == 0 )
        {
            printf("Client disconnected\n");
            break;
        }

        message_counter++;
        printf("The client message #%d: %s\n", message_counter, buffer);

        // Pretend to take some time to attend the request
        //sleep(3);

        sprintf(buffer, "Reply to message #%d\n", message_counter);
        ///// SEND
        // Send a reply to the client
        if ( send(connection_fd, buffer, strlen(buffer)+1, 0) == -1 )
        {
            perror("ERROR: send");
            exit(EXIT_FAILURE);
        }
    }
}
