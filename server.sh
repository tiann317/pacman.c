#!/bin/bash
FILE=server     
if [ -f $FILE ]; then
    rm server
fi
gcc  -Wall -o server game.c -lncurses -lpthread
./server -s -p 8080 -k 4 -n Vasya