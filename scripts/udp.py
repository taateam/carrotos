#!/bin/python3
# udp_client.py
import socket
import time

DST_IP   = "10.0.0.2"   # IP carrotOS
DST_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"UDP client sending to {DST_IP}:{DST_PORT}")

i = 0
while True:
    msg = f"hello {i}".encode()
    sock.sendto(msg, (DST_IP, DST_PORT))
    print(f"{i}: Sent {len(msg)} bytes")
    i += 1
    time.sleep(1)
