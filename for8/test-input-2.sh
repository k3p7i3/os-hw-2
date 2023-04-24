#!/bin/bash
gcc common.h common.c process2.c -o process2 -lpthread -lrt
./process2 5 3 3