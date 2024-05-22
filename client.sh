#!/bin/bash
FILE=client     
if [ -f $FILE ]; then
    rm client
fi
gcc  -Wall -o client game.c -lncurses -lpthread
./client -c -p 8080 -n Petya