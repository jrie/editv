#pragma once

#include "SDL3/SDL_video.h"

#ifdef _WIN32
#define USE_WIDE_CHAR
#endif

#ifdef USE_WIDE_CHAR
#include <wchar.h>
typedef wchar_t VCHAR;
#else
typedef char VCHAR;
#endif

typedef struct {
	const VCHAR* title;
	const VCHAR* path;
	const VCHAR* filter_name;
	const VCHAR* filter;
	const VCHAR* extension;
} edv_file_options;


void edv_init(SDL_Window* window);
const VCHAR* edv_open_file(edv_file_options* opt);
const VCHAR* edv_save_file(edv_file_options* opt);

const size_t edv_clipboard(char *clip);