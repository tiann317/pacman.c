#!/bin/bash
gcc -Wall -Wextra -std=c11 -pedantic -ggdb -o server server.c
gcc -Wall -Wextra -std=c11 -pedantic -ggdb -o client client.c