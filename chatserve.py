#!/usr/bin/env python3

'''
 Program Name: chatserve.py
Programmer Name: Jordan Hamilton
Course Name: CS 372 - Intro to Computer Networks
Last Modified: 2/9/2020
Description: This application takes a parameter for a port on which to lisen for incoming
connections then creates a socket and waits for a client to connect on that socket
If successful, the server displays the received initial message, then displays a prompt
for the server to respond. The server then sends the entered message to the client handle
waits until the client responds then repeats the process of sending messages back and forth
until either the server or client quits by typing "/quit" at the message prompt. If the client
quits, the server continues to listen for new connections. The process is terminated if the
server sends "/quit" to the client
'''
from socket import socket, AF_INET, SHUT_RDWR, SOCK_STREAM, getfqdn
import sys


def main():
    # Listen on any interface.
    host = ''
    # Get the port number entered on the command line
    port = int(sys.argv[1])
    # Set a flag to indicate the server should not terminate. This is updated
    # to True if the server sends a "/quit" message to the client.
    server_shutdown = False
    # A hardcoded handle for the server's "username"
    handle = 'hamiltj2> '
    # The command the server or client would enter to exit the chat.
    quit_cmd = '/quit'
    # A unique string indicating the end of the message being sent to the client.
    end_of_message = '||'


    with socket(AF_INET, SOCK_STREAM) as sock:
        # Attempt to bind the socket to the address and accept incoming connections.
        # Credit: https://docs.python.org/3/library/socket.html#example
        sock.bind((host, port))
        sock.listen(1)
        print( 'Now listening for incoming connections on %s:%d.' % (getfqdn(), port) )

        # Loop indefinitely to accept new incoming connections while the server
        # process is still execting.
        while True:
            conn, addr = sock.accept()

            with conn:
                # When a new connection is accepted, declare and initialize
                # quit_detected as False so we can loop indefinitely with this
                # client and continue sending and receiving messages.
                quit_detected = False
                print( 'Connected to %s:%d.' % (addr[0], addr[1]) )

                while not quit_detected:
                    # Wait to receive the initial message to be sent by the client.
                    data = conn.recv(1024)

                    # If the client sent the quit command, shutdown the conneciton
                    # and break out of the loop to accept a new connection.
                    if data.decode() == quit_cmd:
                        quit_detected = True
                        print( 'Client %s:%d disconnected.' % (addr[0], addr[1]) )
                        conn.shutdown(SHUT_RDWR)
                        break

                    # Print the client's send message.
                    print('%s' % data.decode())

                    # Print the username "handle" prompt on the server.
                    print('%s' % handle, end='')

                    # Read a new message from the server to be sent to the client.
                    message = input()

                    if message == quit_cmd:
                        # If the server sent the shutdown message, only send the
                        # message and the end of message indicator "||" to the
                        # client for easier parsing when the server is disconnecting.
                        # Also set the server_shutdown flag to True to begin cleanup.
                        server_shutdown = True
                        message = message + end_of_message
                    else:
                        # If the message being sent wasn't the quit command,
                        # then prepend the server handle before sending the message.
                        message = handle + message + end_of_message

                    try:
                        # Send the message to the client.
                        conn.sendall(message.encode())
                    except:
                        # Set quit detected to True and display a message on the
                        # server if we're no longer able to send data to the client.
                        print( 'Client %s:%d disconnected.' % (addr[0], addr[1]) )
                        quit_detected = True

                    if server_shutdown:
                        # Begin the shutdown the server after the /quit message
                        # is sent.
                        shutdown_server(sock, conn)

                conn.close()


# This function attempts to close the connection to a client and stops listening
# for incoming connections on the original socket, then exits the process.
def shutdown_server(sock, conn):
    # Close the client connection.
    conn.shutdown(SHUT_RDWR)
    conn.close()

    # Stop listening on the port the server is bound to.
    sock.shutdown(SHUT_RDWR)
    sock.close()

    # Exit the process.
    raise SystemExit(0)


if __name__ == "__main__":
    main()
