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
#include <time.h>

// Include libraries for sockets
#include <netdb.h>
#include <arpa/inet.h>

#define SERVICE_PORT 8642
#define MAX_QUEUE 5
#define BUFFER_SIZE 1024
#define DECK_SIZE 52
#define CARD_SIZE 5
#define NAME_SIZE 20
#define HAND_SIZE 100
#define IN_USE 99


void usage(char * program);
void startServer(char * port);
void waitForConnections(int server_fd);
void communicationLoop(int connection_fd);

//own variables and functions
int deck [DECK_SIZE];
void deckSetup();
char * traducirCarta(int x);
void translate(int x, char[CARD_SIZE]);
int hit();
void printDeck();

int main(int argc, char * argv[]){

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
        //printf("I am the parent, continuing loop...\n");
        continue;

      } else if (pid == 0){ //child
        //printf("child #%d forked\n", a);

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
    //printf("Parent waiting for children to terminate\n");
    for(int a=0; a < players; a++){
      //printf("At least i'm in the loop\n");
      pid_t  pid;
      int status = 0;
      pid = wait(&status);
      if (WIFEXITED(status))
        {
            printf("Child finished with status: %d\n", WEXITSTATUS(status));
        }
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
    char playerName[NAME_SIZE];

    bzero(buffer, BUFFER_SIZE);

    deckSetup();
    int cardNumber = 0;
    char cardString[CARD_SIZE];
    int bet = 0;
    char hand[HAND_SIZE];
    char looper;
    char message[BUFFER_SIZE];

    srand(time(NULL)); //needed for radomizing things

    while (1)
    {
        //LISTEN
        // Clear the buffer
        bzero(buffer, BUFFER_SIZE);
        // Read the message from the client
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

        if (message_counter == 0) { //deal cards
          sprintf(playerName, "%s", buffer); //save player name
          strtok(playerName, "\n"); //clean up input (fgets appends a \n at the end)

          //first card
          cardNumber = hit();
          translate(cardNumber, cardString);
          strcat(hand, cardString);

          //second card
          cardNumber = hit();
          translate(cardNumber, cardString);
          strcat(hand, cardString);

          //initial hand
          printf("Dealt %s to %s.\n", hand, playerName);

          sprintf(buffer, "%s", hand); //write to buffer

        } else if (message_counter == 1) { //receive bet amount, ask to hit or stand
          bet = atoi(buffer); //save bet amount
          printf("%s has bet %d\n", playerName, bet);
          sprintf(buffer, "Confirmed. You have bet $%d, %s.", bet, playerName); //write to buffer

        } else { //hit or stand loop, message_counter > 1
          sscanf(buffer, "%c", &looper);
          if (looper == 'h') { //deal another card

            //get new card
            cardNumber = hit();
            translate(cardNumber, cardString);
            strcat(hand, cardString);

            printf("Dealt %s to %s.\n", cardString, playerName);

            //no message to send
            sprintf(message, "");

          } else if (looper == 's'){
            printf("%s has decided to stand. Dealing the dealer's cards...\n", playerName);
            //hand doesn't change
            sprintf(message, "finished");

          } else {
            printf("Player must answer either h or s\n");
            //hand doesn't change
            sprintf(message, "%s", "Please answer either h or s");
          }
          sprintf(buffer, "%s %s", hand, message); //write to buffer
        }

        message_counter++;
        ///// SEND
        // Send a reply to the client
        if ( send(connection_fd, buffer, strlen(buffer)+1, 0) == -1 )
        {
            perror("ERROR: send");
            exit(EXIT_FAILURE);
        }

    }
}

void deckSetup(){

  //fill in the deck
  for (int a = 0; a < DECK_SIZE; a++) {
    deck[a] = a;
  }

  //printDeck();

  //shuffle the deck
  int b, temp;
  for (int a = 0; a < DECK_SIZE; a++){
      temp = deck[a]; //swaps each card at least once
      b = (rand() % DECK_SIZE);
      deck[a] = deck[b];
      deck[b] = temp;
  }
}

//get a card from the deck
int hit(){
  int z;
  z = (rand() % DECK_SIZE); //if deck[z] is zero, that card is already in use
  while (deck[z] == IN_USE) //checks the availability of the card
  {
      z = (rand() % DECK_SIZE); //asks for another card
  }
  int temp = deck[z];
  deck[z] = IN_USE; //card was good, now set that one 52 so we don't choose it again
  return temp; //return the actual card value
}

//translates from a numeric value to a nicely-formatted string
void translate(int x, char card[CARD_SIZE]){
    int valor;
    static char str[CARD_SIZE];
    //NO MODIFICAR LOS OPERADORES LÓGICOS!
    if (x <= 13) //CORAZONES ♥
    {
        valor = (x % 13);
        sprintf(card, "[%s♥]", traducirCarta(valor));
    }
    else if (13 < x && x <= 26) //DIAMANTES ♦
    {
        valor = (x % 13);
        sprintf(card, "[%s♦]", traducirCarta(valor));
    }
    else if (26 < x && x <= 39) //ESPADAS ♠
    {
        valor = (x % 13);
        sprintf(card, "[%s♠]", traducirCarta(valor));
    }
    else //TREBOLES ♣
    {
        valor = (x % 13);
        sprintf(card, "[%s♣]", traducirCarta(valor));
    }
}

void printDeck(){
    for (int a = 0; a < DECK_SIZE; a++){
        printf("%d / ", deck[a]);
    }
}

//switches from 0-12 values to cards
char * traducirCarta(int x)
{
    switch (x)
    {
        case 0:
            return "K";
            break;
        case 1:
            return "A";
            break;
        case 2:
            return "2";
            break;
        case 3:
            return "3";
            break;
        case 4:
            return "4";
            break;
        case 5:
            return "5";
            break;
        case 6:
            return "6";
            break;
        case 7:
            return "7";
            break;
        case 8:
            return "8";
            break;
        case 9:
            return "9";
            break;
        case 10:
            return "10";
            break;
        case 11:
            return "J";
            break;
        case 12:
            return "Q";
            break;
        default:
            return "N";
            break;
    }
}
