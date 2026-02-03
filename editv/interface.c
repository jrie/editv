#include "interface.h"
#include <string.h>
#include <stdio.h>
#include <SDL3/SDL.h>

void edv_init(SDL_Window* window) {
}

int edv_open_file(const char* lastOpenFile, char* buffer, size_t length) {

	printf("Insert File Open Location:\n");

	fgets(buffer, length, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	return 0;
}

int edv_save_file(const char* lastOpenFile, char* buffer, size_t length) {

	printf("Insert File Save Location:\n");

	fgets(buffer, length, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	return 0;
}

const size_t edv_clipboard(char** clip) {

	char* c = SDL_GetClipboardText();

	return 0;
}

