# Sparkle-Engine  

Vulkan based rendering engine for Windows and Linux 

## Planned
* Integrate Logger library to fix the current logging mess  
* Shadowmaps & softshadows
* integrate Scripting language for main application logic  
* separate editor and game view  

## Dependencies  
* GCC/Clang/MSVC with C++17 support  
* Vulkan SDK  
* glslang package on linux
* [assimp](https://github.com/assimp/assimp)


## Setup assimp on windows  
* Build assimp for your MSVC version and copy it to desired location (for ex. C:\Tools\assimp)  
* set Windows Environment variable "ASSIMP_ROOT_DIR" to the Path to your assimp folder  
* clear existing cache for cmake (remove CMakeCache.txt or delete the cache folder)  
* CMake should detect your assimp installation now

## Build on Arch-Linux with clang
* Install build requirements:  
```
sudo pacman -S base-devel cmake vulkan-headers vulkan-validation-layers clang libc++ libc++abi libc++experimental glfw-x11
```  
* Install assimp from aur, but make sure to select the package form *aur* and not *extra*   
```
yay assimp
```   
* create build directory, configure with cmake and build:  
```
mkdir build && cd build && CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j4
```
* Done! Set a correct object file as level in settings.ini and run ```./sparkle-engine```  
