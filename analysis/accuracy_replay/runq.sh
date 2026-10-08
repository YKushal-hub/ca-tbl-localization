#!/bin/bash
# usage: runq.sh <Q> <cell> <seed> flags...
Q=$1; shift
export CATBL_QUSCALE=$Q
exec ./runcell.sh "$@"
