#!/bin/bash


# Store the argument in a variable
input=$1
output=$2

nasm -f elf64 $input -o temp.o
ld temp.o -o $output
rm temp.o
