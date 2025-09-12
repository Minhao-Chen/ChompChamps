#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Uso: $0 <flags master...>"
    echo
    echo "Ejemplo:"
    echo " $0 -w 20 -h 15 -d 200 -t 10 -s 42 -v ./bin/view -p ./bin/player ./bin/player"
    exit 1
fi

make clean
make all
./bin/master "$@"
