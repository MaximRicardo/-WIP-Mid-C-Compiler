#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

$SCRIPT_DIR/../bin/mcc $SCRIPT_DIR/a.c $SCRIPT_DIR/a.s
$SCRIPT_DIR/assemble.sh
