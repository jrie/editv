# editv
A minimal C based text editor

This a project with minimal dependencies, Graphics are rendered with [SDL3](https://wiki.libsdl.org/SDL3) and everything else is either pure C or platform dependant.

It is currently very basic, but supports everything needed in a text editor

### Features

- Open/Save files
- 4 directional Cursor Movement
- Text Scrolling
- Pasting from clipboard

### In Development

- Text selection
- Copying
- Undo/Redo
- Text Finding/Pattern Matching


## Usage 

Open to immediately begin writing

To view options hold 'ctrl' and press a corresponding key 
'o' - Open file
's' - save file
'v' - paste from clipboard
'q' - quit


## Building

The only dependency this project has is [SDL3](https://wiki.libsdl.org/SDL3), so that will need to be installed to build.


#### On non-Windows platforms, define PORTABLE=1 to generate a non platform specific build that takes file path input through the console rather than a window  

### Visual Studio

Ensure SDL3 is installed into C:/SDL (https://wiki.libsdl.org/SDL3/README-cmake)

Open and build editv.sln

#### If any errors occur, you can manually link SDL by following thse steps:  

Adding your SDL3 installation's include folder to Project->Properties->editv Properties->C/C++->Additional Include Directories  

Adding your SDL3 installation's SDL3.lib file to Project->Properties->editv Properties->Linker->Additional Dependencies (note this has to be the actual SDL3.lib file, and not the lib folder)  


### CMake

Ensure [CMake](https://cmake.org/) is installed

In the source directory, create a folder named 'build'.  
Then run these commands:  
```
cd build
cmake ..
cmake --build .
```

### gcc

Ensure [GCC](https://gcc.gnu.org/) is installed

In the source directory, run this command to build with GCC  
```
gcc editv/main.c editv/storage.c -o build/editv -I/path/to/SDL3/include -L/path/to/SDL3/lib -lSDL3
```

