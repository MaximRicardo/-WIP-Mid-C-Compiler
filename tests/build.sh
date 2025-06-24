#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/../bin/mcc $SCRIPT_DIR/a.c -o $SCRIPT_DIR/a.s -mccir $SCRIPT_DIR/a.ir -O --pedantic -Werror --no-const-folding

# this'll need to wait for when i make a MidASM assembler and emulator.
# $SCRIPT_DIR/assemble.sh
