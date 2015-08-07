#!/bin/bash

set -- `getopt -q ab:cd "$@"`

while [ -n "$1" ]
do
    case $1 in
    -a) echo "this is the a option";;
    -b) 
    echo "this is the b option"
    echo "b option value is $2"
    shift;;
    -c) echo "this is the c option";;
    -d) echo "this is the d option";;
    --) shift 
        break;;
    esac
    shift
done

for option_value in "$@" 
do
    echo "$option_value"
done
