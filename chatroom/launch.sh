#!/bin/bash

# start server
gnome-terminal -- bash -c "./server 3000; exec bash"

# launch clients
gnome-terminal -- bash -c "./client 127.0.0.1 3000; exec bash"
gnome-terminal -- bash -c "./client 127.0.0.1 3000; exec bash"