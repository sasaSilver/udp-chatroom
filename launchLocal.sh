#!/bin/bash
# Builds the project and starts the server and two clients

cd src

# Build the project
make

# Start server
gnome-terminal -- bash -c "./server 3000; exec bash"

# Launch clients
gnome-terminal -- bash -c "./client 127.0.0.1 3000; exec bash"
gnome-terminal -- bash -c "./client 127.0.0.1 3000; exec bash"
