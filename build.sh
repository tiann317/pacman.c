#!/bin/bash
gcc -Wall -Wextra -std=c11 -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L -pedantic -ggdb -o main main.c -lncurses
