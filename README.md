# Sparkle-Engine  

Vulkan based rendering engine.

## Planned
* use vulkan compute shader for frustum culling  
* generate new command buffers in compute shader  
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