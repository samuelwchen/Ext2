#! /bin/bash

clear
cp diskimage mydisk
gcc -g -m32 *.c
./a.out mydisk
