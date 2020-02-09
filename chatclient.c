/***********************************************************************************************
** Program Name: chatclient
** Programmer Name: Jordan Hamilton
** Course Name: CS 372 - Intro to Computer Networks
** Last Modified: 2/9/2020
** Description: This application takes a parameter for a host to connect to and the port to
** connect to on that host, then creates a socket and attempts to establish a connection with
** that host. If successful, the user is then prompted to enter a username, then an initial
** message is sent to the server with the port number of the host the client connected to,
** with the username prepended to the message. The client then waits until the server responds
** then repeats the process of sending messages back and forth until either the server or client
** quits by typing "/quit" at the message prompt.
***********************************************************************************************/

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void connectToServer(const struct sockaddr_in*, int*);
void defineServer(struct sockaddr_in*, const char*, const char*);
void error(const char*);
void initChat(char[], const int*, const char*, const char*, const int*);
void receiveStringFromSocket(const int*, char[], char[], const int*, const char[]);
void sendStringToSocket(const int*, const char[]);

int main(int argc, char* argv[]) {
  int socketFD, quitReceived = 0, charsEntered = -5;
  char *username = NULL, *message = NULL;
  size_t usernameSize = 0, messageSize = 0;
  const int bufferSize = 1024, messageFragmentSize = 10;
  char buffer[bufferSize], messageFragment[messageFragmentSize];
  const char handleSuffix[] = "> ";
  const char quitCmd[] = "/quit";
  const char endOfMessage[] = "||";
  struct sockaddr_in serverAddress;

  // Check usage to ensure the hostname and port are passed as command line arguments.
  if (argc != 3) {
    fprintf(stderr, "Correct command format: %s HOSTNAME PORT\n", argv[0]);
    exit(2);
  }

  // Create a struct containing server information.
  defineServer(&serverAddress, argv[1], argv[2]);

  //Connect to the server as described in the serverAddress struct using the passed socket file descriptor.
  connectToServer(&serverAddress, &socketFD);

  // Prompt the user to provide a username with at least 1 character and no more than 10.
  do {
    if (username) {
      free(username);
      username = NULL;
    }

    fprintf(stdout, "Please enter a username: ");
    charsEntered = getline(&username, &usernameSize, stdin);
    username[charsEntered - 1] = '\0';

    if (charsEntered < 1 || charsEntered > 11)
      fprintf(stdout, "Your username must be between 1 and 10 characters.\n");

  } while (charsEntered > 11);

  // Append "> " to the end of the username to prepend to each message
  strncat(username, handleSuffix, strlen(handleSuffix));

  // Send the initialization message to server
  initChat(buffer, &bufferSize, username, argv[2], &socketFD);

  // Repeat receiving and then sending messages until a /quit message is either detected from stdin or received from the
  // server.
  do {
    // Ensure that getline can allocate a buffer for messages entered by the user.
    if (message) {
      free(message);
      message = NULL;
    }

    // Clear out the buffer used for sending and receiving messages.
    memset(buffer, '\0', sizeof(buffer));
    receiveStringFromSocket(&socketFD, buffer, messageFragment, &messageFragmentSize, endOfMessage);

    if (strcmp(buffer, quitCmd) == 0) {
      // If the string sent by the server was "/quit", inform the user and exit the while loop and clean up the application.
      fprintf(stdout, "%s:%s is shutting down.\n", argv[1], argv[2]);
      break;
    }
    else {
      // Output the message sent by the server to the user's screen.
      fprintf(stdout, "%s\n", buffer);
    }

    // Display a prompt for the user to reply to the server.
    fprintf(stdout, "%s", username);

    // Get the user's input
    charsEntered = getline(&message, &messageSize, stdin);
    message[charsEntered - 1] = '\0';

    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));

    if (strcmp(message, quitCmd) != 0) {
      // If the user didn't type quit, send the server both the username and the message the user entered.
      strcpy(buffer, username); // Copy the handle into the buffer
      strcat(buffer, message); // Append the message to the buffer
    } else {
      // Send only the message so the server can more easily parse situations when the client is disconnecting.
      // Use a flag (quitReceived) to exit the loop and clean up the application.
      strcpy(buffer, message); // Copy only the message into the buffer
      quitReceived = 1;
    }

    // Send the user's message to the server.
    sendStringToSocket(&socketFD, buffer);

  } while (!quitReceived);

  // Close the socket.
  close(socketFD);

  // Free any memory allocated for messages entered by the user.
  if (message) {
    free(message);
    message = NULL;
  }

  // Free memory for the user's provided username.
  if (username) {
    free(username);
    username = NULL;
  }

  // Quit the program.
  return(0);
}

/* This function takes a pointer to a sockaddr_in struct and a pointer to a socket file descriptor, then attempts to
 * create a socket and connects to the server via that socket. Credit: CS 344 (Operating Systems 1) lecture video 4.2. */
void connectToServer(const struct sockaddr_in* serverAddress, int* socketFD) {
  // Set up the socket
  *socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (*socketFD < 0)
    error("An error occurred creating a socket");

  // Connect to server
  if (connect(*socketFD, (struct sockaddr*) serverAddress, sizeof(*serverAddress)) < 0) // Connect socket to address
    error("An error occurred connecting to the server");
}

/* This function takes a pointer to a sockaddr_in struct and two strings representing the server address and port, then
 * attempts to declare the necessary properties in the struct to allow a socket to be created passing that struct in the
 * connectToServer function. Credit: CS 344 (Operating Systems 1) lecture video 4.2. */
void defineServer(struct sockaddr_in* serverAddress, const char* host, const char* port) {
  int portNumber;
  struct hostent* serverHostInfo;

  // Set up the server address struct
  memset((char*) serverAddress, '\0', sizeof(*serverAddress)); // Clear out the address struct
  portNumber = atoi(port); // Get the port number, convert to an integer from a string
  serverAddress->sin_family = AF_INET; // Create a network-capable socket
  serverAddress->sin_port = htons(portNumber); // Store the port number
  serverHostInfo = gethostbyname(host); // Convert the machine name into a special form of address

  if (serverHostInfo == NULL) {
    fprintf(stderr, "An error occurred defining a server address.\n");
    exit(2);
  }

  // Copy in the address
  memcpy((char*) &serverAddress->sin_addr.s_addr, (char*) serverHostInfo->h_addr, serverHostInfo->h_length);
}

// Error function used for reporting issues
void error(const char* msg) {
  perror(msg);
  exit(2);
}

/* This function takes a char array, a pointer to an int indicating that array's size, a string with the client's
 * username, the message to be sent (in this case, the port number of the server specified at the command line
 * when executing the client), and finally the socket file descriptor that should be used to send the message.
 * The buffer is cleared out and filled with the username followed by that port to meet the requirement when initiating
 * a chat session with the server. */
void initChat(char buffer[], const int* bufferSize, const char* username, const char* message, const int* socketFD) {
  memset(buffer, '\0', *bufferSize); // Clear out the buffer
  strcpy(buffer, username);
  strcat(buffer, message);
  sendStringToSocket(socketFD, buffer);
}

/* Takes a socket, a message buffer, a smaller array to hold characters as they're read, the size of the array, and a
 * small string used by client and server to indicate the end of a message. The function loops through until the
 * substring is found, as seen in the Network Clients video for block 4, repeatedly adding the message fragment
 * to the end of the message. The substring that marks the end of the message is then replaced with a null terminator. */
void receiveStringFromSocket(const int* establishedConnectionFD, char message[], char messageFragment[], const int* messageFragmentSize, const char endOfMessage[]) {
  int charsRead = -5;
  long terminalLocation = -5;

  while (strstr(message, endOfMessage) == NULL) {
    memset(messageFragment, '\0', *messageFragmentSize);
    charsRead = recv(*establishedConnectionFD, messageFragment, *messageFragmentSize - 1, 0);

    // Exit the loop if we either don't read any more characters when receiving, or we failed to retrieve any characters
    if (charsRead <= 0)
      break;

    strcat(message, messageFragment);
  }

  /* Find the terminal location using the method in the Network Clients video from Block 4
   * then set a null terminator after the actual message contents end. */
  terminalLocation = strstr(message, endOfMessage) - message;
  message[terminalLocation] = '\0';
}

/* Takes a pointer to a socket, followed by a string to send via that socket,
 * then loops to ensure all the data in the string is sent. */
void sendStringToSocket(const int* socketFD, const char message[]) {
  int charsWritten;
  // Send message to server
  charsWritten = send(*socketFD, message, strlen(message), 0); // Write to the server
  if (charsWritten < 0)
    error("An error occurred writing to the socket");

  while (charsWritten < strlen(message)) {
    int addedChars = 0;
    // Write to the server again, starting from one character after the most recently sent character
    addedChars = send(*socketFD, message + charsWritten, strlen(message) - charsWritten, 0);
    if (addedChars < 0)
      error("An error occurred writing to the socket");

    // Exit the loop if no more characters are being sent to the server.
    if (addedChars == 0)
      break;

    // Add the number of characters written in an iteration to the total number of characters sent in the message
    charsWritten += addedChars;
  }
}