#!/bin/bash
mkdir -p build

# TODO: a way to skip this SDL building code for MUCH faster builds
if [ -d "SDL" ]; then
    cd SDL
    cmake -S . -B build -DSDL_SHARED=OFF -DSDL_STATIC=ON && cmake --build build
    cd ..
else
    echo "SDL directory not found!"
fi

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

clang ../main.c -I ../SDL/include -o demongus ../SDL/build/libSDL3.a -lm

echo "Created $BUILD_TYPE build"

exit 0
