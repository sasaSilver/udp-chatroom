@echo off
:: Builds the project and starts the server and two clients

cd src

:: Build the project
make

:: Start server
start cmd /k ".\server 3000"

:: Launch clients
start cmd /k ".\client 127.0.0.1 3000"
start cmd /k ".\client 127.0.0.1 3000"
