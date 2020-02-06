#!/usr/bin/env python3

from socket import socket, AF_INET, SOCK_STREAM
import sys


def main():
    host = ''
    port = int(sys.argv[1])

    with socket(AF_INET, SOCK_STREAM) as sock:
        sock.bind((host, port))
        sock.listen(1)

        while True:
            conn, addr = sock.accept()
            with conn:
                print('Connected to ', addr)
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    conn.sendall(data)


if __name__ == "__main__":
    main()
