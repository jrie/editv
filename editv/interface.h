#pragma once

#include "SDL3/SDL_video.h"


void edv_init(SDL_Window* window);
int edv_open_file(const char* lastOpenFile, const char* buffer, size_t length);
int edv_save_file(const char* lastOpenFile, const char* buffer, size_t length);

const size_t edv_clipboard(char **clip);