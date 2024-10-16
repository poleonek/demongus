#!/bin/bash

set -e

shopt -s expand_aliases

case $1 in
    -b|--build-type)
        BUILD_TYPE=$2
        ;;
esac

if [[ -z $BUILD_TYPE ]]; then
    BUILD_TYPE="RELEASE"
fi

case $BUILD_TYPE in
    DEBUG)
        alias clang="clang -O0 -g"
        ;;
    RELEASE)
        alias clang="clang"
        ;;
    *)
        echo "Accepted args for -b|--build-type are only RELEASE or DEBUG."
        echo "$BUILD_TYPE was provided."
        exit -1
        ;;
esac


cd build

clang ../main.c -o main -lSDL3
