# demongus
A game/engine. Initially inspired by [Among Us](https://store.steampowered.com/app/945360/Among_Us/).

Requires SDL3

## CLONING THE PROJECT
This repository uses git submodules to include SDL3.

To download the repo and SDL3 at the same time run:
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
cd libs\SDL
cmake -S . -B build\win -DSDL_STATIC=ON && cmake --build build\win
cd ..\..

cd libs\SDL_image
cmake -S . -B build\win -DSDLIMAGE_VENDORED=OFF -DBUILD_SHARED_LIBS=OFF "-DSDL3_DIR=..\SDL\build\win" && cmake --build build\win
cd ..\..
```

# Resources
## Collision
The game uses SAT algorithm for collision detection.
Two links that I found especially useful while researching how to implement it:
- [Collision Detection with SAT (Math for Game Developers) by pikuma](https://www.youtube.com/watch?v=-EsWKT7Doww)
- [N Tutorial A – Collision Detection and Response](https://www.metanetsoftware.com/2016/n-tutorial-a-collision-detection-and-response)
