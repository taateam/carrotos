#!/bin/bash
sudo ip tuntap add dev tap0 mode tap
sudo ip link set tap0 up
sudo ip addr add 10.0.0.1/24 dev tap0
sudo ufw allow in on tap0
sudo ufw allow out on tap0
sudo sysctl -w net.ipv4.ip_forward=1
