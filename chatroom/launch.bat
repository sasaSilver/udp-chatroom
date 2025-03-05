@echo off
:: start server
start cmd /k "server 3000"
:: launch clients
start cmd /k "client 127.0.0.1 3000"
start cmd /k "client 127.0.0.1 3000"