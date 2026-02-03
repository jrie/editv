#include "interface.h"
#include <string.h>

#ifdef _WIN32

#include <windows.h>
#include "SDL3/SDL_video.h"


HWND hwnd;


//platform specific information
//https://github.com/libsdl-org/SDL/blob/main/docs/README-migration.md#sdl_syswmh
void edv_init(SDL_Window *window) {

	hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);


}

const VCHAR* edv_open_file(edv_file_options* opt) {

}

const VCHAR* edv_save_file(edv_file_options* opt) {

}

const size_t edv_clipboard(char** clip){

	if (!IsClipboardFormatAvailable(CF_TEXT))
		return;
	if (!OpenClipboard(NULL))
		return;


	HANDLE hglb = GetClipboardData(CF_TEXT);

	size_t newsize = 0;

	char* clip_ptr = GlobalLock(hglb); //pointer to wide string
    if (clip_ptr != NULL)
    {


		newsize = strlen(clip_ptr) + 1;

		*clip = malloc(newsize);

		memcpy(*clip, clip_ptr, newsize);

		//wcstombs_s(&convertedChars, *clip, newsize, lptstr, _TRUNCATE);

        GlobalUnlock(hglb);
    }

	CloseClipboard();

	return newsize;
}

#endif