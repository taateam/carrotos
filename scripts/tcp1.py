#!/bin/python3
import socket
import time

SERVER_IP   = "10.0.0.2"   #
SERVER_PORT = 12345

cli = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

print(f"Connecting to {SERVER_IP}:{SERVER_PORT}")
cli.connect((SERVER_IP, SERVER_PORT))
print("Connected")

payload = b"A" * 64   #
cli.sendall(payload)
print(f"Sent {len(payload)} bytes")
time.sleep(5)
cli.sendall(payload)
print(f"Sent {len(payload)} bytes")
time.sleep(12)
#
cli.shutdown(socket.SHUT_WR)
time.sleep(1)

cli.close()
print("Closed")
