#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/../bin/mcc $SCRIPT_DIR/a.c -o $SCRIPT_DIR/a.s -mccir $SCRIPT_DIR/a.ir -O --pedantic -Werror
$SCRIPT_DIR/assemble.sh
