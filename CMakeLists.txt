cmake_minimum_required(VERSION 3.28)

project(
  editv 
  VERSION 0.0.1
  DESCRIPTION "A minimal C based Text Editor"
  LANGUAGES C
)

set(SOURCE_FILES
  editv/main.c
  editv/storage.c
)

add_compile_options(
  -Wall
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)


add_executable(editv ${SOURCE_FILES})

target_include_directories(editv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/editv)

target_link_libraries(editv PUBLIC
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
)

install(TARGETS editv DESTINATION bin)