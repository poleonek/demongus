# demongus
Among us clone

Requires SDL3

## CLONING PROJECT
Repo uses git submodules to include SDL3.

To download repo and SDL3 at the same time run:
```bash
git clone --recurse-submodules git@github.com:poleonek/demongus.git
```
To clone repo and update submodules separately run:
```bash
git clone git@github.com:poleonek/demongus.git
git submodule update --init --recursive
```

## BUILD
### Linux
Dependencies: cmake, clang
```bash
./build.sh
```
```bash
./build.sh --build-type DEBUG
```
