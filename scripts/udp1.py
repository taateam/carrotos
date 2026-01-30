#!/bin/python3
# udp_server.py
import socket

UDP_IP = "0.0.0.0"     #
UDP_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"UDP server listening on {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(4096)  # buffer 4KB
    print(f"Received {len(data)} bytes from {addr}: {data}")

    #
    sock.sendto(b"ACK", addr)
