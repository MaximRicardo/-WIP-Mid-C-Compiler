#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/../bin/mcc $SCRIPT_DIR/a.c -o $SCRIPT_DIR/a.s -O
$SCRIPT_DIR/assemble.sh
