#!/bin/bash
gcc common.h common.c process1.c -o process1 -lpthread -lrt
./process1 4 2 3