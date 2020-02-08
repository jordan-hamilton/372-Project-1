//
// Created by Jordan Hamilton on 2/8/20.
//

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

void error(const char* msg);
void receiveStringFromSocket(const int*, char[], char[], const int*, const char[]);
void sendStringToSocket(const int*, const char[]);

int main(int argc, char* argv[]) {
  int socketFD, portNumber, quitReceived = 0, charsEntered = -5;
  char *username = NULL, *message = NULL;
  size_t usernameSize = 0, messageSize = 0;
  int bufferSize = 1024, messageFragmentSize = 10;
  char buffer[bufferSize], messageFragment[messageFragmentSize];
  const char *quitCmd = "/quit";
  char endOfMessage[] = "||";
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;

  // Check usage & args
  if (argc != 3) {
    fprintf(stderr, "Correct command format: %s HOSTNAME PORT\n", argv[0]);
    exit(2);
  }

  // Set up the server address struct
  memset((char*) &serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
  portNumber = atoi(argv[2]); // Get the port number, convert to an integer from a string
  serverAddress.sin_family = AF_INET; // Create a network-capable socket
  serverAddress.sin_port = htons(portNumber); // Store the port number
  serverHostInfo = gethostbyname(argv[1]); // Convert the machine name into a special form of address

  if (serverHostInfo == NULL) {
    fprintf(stderr, "An error occurred defining a server address.\n");
    exit(2);
  }
  memcpy((char*) &serverAddress.sin_addr.s_addr, (char*) serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

  // Set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
  if (socketFD < 0)
    error("An error occurred creating a socket");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
    error("An error occurred connecting to the server");

  do {
    free(username);
    username = NULL;

    fprintf(stdout, "Please enter a username: ");
    charsEntered = getline(&username, &usernameSize, stdin);
    username[charsEntered - 1] = '\0';

    if (charsEntered > 11)
      fprintf(stdout, "Your username must be 10 characters or fewer.\n");

  } while (charsEntered > 11);

  strcat(username, "> ");

  // Send message to server
  memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer
  strcpy(buffer, username);
  strcat(buffer, argv[2]);
  sendStringToSocket(&socketFD, buffer);

  do {
    free(message);
    message = NULL;

    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
    receiveStringFromSocket(&socketFD, buffer, messageFragment, &messageFragmentSize, endOfMessage);
    fprintf(stdout, "%s\n", buffer);

    fprintf(stdout, "%s", username);

    charsEntered = getline(&message, &messageSize, stdin);
    message[charsEntered - 1] = '\0';

    if (strcmp(message, quitCmd) == 0) {
      quitReceived = 1;
    } else {
      memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
      strcpy(buffer, username); // Copy the handle into the buffer
      strcat(buffer, message);
      sendStringToSocket(&socketFD, buffer);
    }
  } while (!quitReceived);

  close(socketFD); // Close the socket
  return(0);
}

// Error function used for reporting issues
void error(const char* msg) {
  perror(msg);
  exit(2);
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
    if (charsRead == 0)
      break;
    if (charsRead == -1)
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