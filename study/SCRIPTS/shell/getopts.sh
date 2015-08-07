#!/bin/bash

while getopts ab:cd opt
do
    case $opt in
    a)
    echo "I see a option"
    ;;
    c) echo "I see c option";;
    d) echo "I see d option";;
    b)
    echo "I see b option and the value is $OPTARG"
    ;;
    esac
done

shift $[ $OPTIND-1 ]

for param in "$@"
do
    echo "I see param $param"
done
