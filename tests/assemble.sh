#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

nasm -f elf64 $SCRIPT_DIR/a.s -o $SCRIPT_DIR/a.o -g
gcc -no-pie -m64 $SCRIPT_DIR/a.o -o $SCRIPT_DIR/a -g
