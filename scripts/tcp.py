#!/bin/python3
# tcp_server.py
import socket

LISTEN_IP   = "0.0.0.0"   #
LISTEN_PORT = 12345

srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

srv.bind((LISTEN_IP, LISTEN_PORT))
srv.listen(5)

print(f"TCP server listening on {LISTEN_IP}:{LISTEN_PORT}")

while True:
    conn, addr = srv.accept()
    print(f"Accepted connection from {addr}")

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                print("Client closed")
                break
            print(f"Received {len(data)} bytes: {data!r}")
    finally:
        conn.close()
