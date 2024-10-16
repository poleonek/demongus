#!/bin/bash

set -e

shopt -s expand_aliases

alias clang="clang -g"

cd build
clang ../main.c -o main -lSDL3
