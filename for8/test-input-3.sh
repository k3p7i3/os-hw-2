#!/bin/bash
gcc common.h common.c process3.c -o process3 -lpthread -lrt
./process3 4 5 3