# editv
A minimal C based text editor

This a project with minimal dependencies, Graphics are rendered with [SDL3](https://wiki.libsdl.org/SDL3) and everything else is pure C magic.  
It is currently very basic, but supports all the basic operations needed in a text editor


![screenshot](https://github.com/nimrag-b/editv/blob/main/editvss.png)

### Features

- Open/Save files
- 4 directional Cursor Movement
- Text Scrolling
- Pasting from clipboard
- Undo/Redo

### In Development

- Text selection
- Copying
- Text Finding/Pattern Matching


## Usage 

Open to immediately begin writing

#### To view options hold 'ctrl' and press a corresponding key   
'o' - Open file  
's' - save file  
'v' - paste from clipboard  
'z' - undo  
'y' - redo  
  
#### To view alt options press 'ctrl' + 'shift'  
's' - save as  
'q' - quit  
  
You can also edit config.cfg (generated when first loaded) for some simple customisation options  

## Building

To build this project you will need [SDL3](https://wiki.libsdl.org/SDL3) and [SDL3_ttf](https://wiki.libsdl.org/SDL3_ttf/FrontPage)


### Visual Studio

Ensure SDL3 is installed into C:/SDL (https://wiki.libsdl.org/SDL3/README-cmake)

Open and build editv.sln

#### If any errors occur, you can manually link SDL by following thse steps:  

Adding your SDL3 and SDL3_ttf installation's include folders to Project->Properties->editv Properties->C/C++->Additional Include Directories  

Adding your SDL3 and SDL3_ttf installation's SDL3.lib and SDL3_ttf.lib files to Project->Properties->editv Properties->Linker->Additional Dependencies (note this has to be the actual SDL3.lib file, and not the lib folder)  


### CMake

Ensure [CMake](https://cmake.org/) is installed, as well as the SDL packages into somewhere CMake can find

In the source directory, create a folder named 'build'.  
Then run these commands:  
```
cd build
cmake ..
cmake --build .
```

To disable the console on Windows, uncomment ` #target_link_options(editv PRIVATE -mwindows) ` in CMakeLists.txt

### gcc

Ensure [GCC](https://gcc.gnu.org/) is installed

In the source directory, run this command to build with GCC, inserting your paths to SDL and SDL_ttf
```
gcc editv/main.c editv/storage.c editv/config.c -o editv/editv -I/path/to/SDL3/include -L/path/to/SDL3/lib -I/path/to/SDL3_ttf/include -L/path/to/SDL3_ttf/lib -lSDL3 -lSDL3_ttf
```




An example build command for Windows where SDL and SDL_ttf are in C:/SDL and C:/SDL_ttf would look like this
```
gcc editv/main.c editv/storage.c editv/config.c -o editv/editv -IC:\SDL\include -LC:\SDL\lib\x64\SDL3.lib -IC:\SDL3_ttf\include -LC:\SDL_ttf\lib\x64\SDL3_ttf.lib -lSDL3 -lSDL3_ttf
```

