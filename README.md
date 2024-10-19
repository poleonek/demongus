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
On Linux: `./build.sh`

On Windows: `./build.bat`

Example commands:
```bash
./build.sh sdl game
```
```bash
./build.sh game
```
```bash
./build.sh sdl game release
```
### Windows SDL build workaround
Building CMake SDL from .bat file seems to be broken.
What works from me is calling SDL build commands manually from Developer pwsh.exe (new powershell + cl compiler).
Sorry about that, would be nice to fix this.
```bat
cd SDL
cmake -S . -B build_win && cmake --build build_win
cd ..
```
