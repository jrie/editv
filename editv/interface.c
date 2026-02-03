#include "interface.h"
#include <string.h>
#include <stdio.h>

#if(PORTABLE == 1)


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

	return 0;
}


#elif defined (_WIN32)

#include <windows.h>
#include <shobjidl_core.h>
#include "SDL3/SDL_video.h"


HWND hwnd;


//platform specific information
//https://github.com/libsdl-org/SDL/blob/main/docs/README-migration.md#sdl_syswmh
void edv_init(SDL_Window *window) {

	hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);


}

int edv_open_file(const char* lastOpenFile, const char* buffer, size_t length) {

	OPENFILENAMEA ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
	HWND hwnd = 0;              // owner window
	HANDLE hf;              // file handle

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;

	ofn.nMaxFileTitle = 0;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (lastOpenFile != NULL) {

		char dirBuf[256];
		char filenameBuf[256];
		char extBuf[256];

		//todo - error handling
		_splitpath_s(lastOpenFile, NULL, 0, dirBuf, 256, filenameBuf, 256, extBuf, 256);

		
		if ((strlen(filenameBuf) + strlen(extBuf)) > 256) {
			//file buffer too big
			return NULL;
		}

		wchar_t *res = strcat(filenameBuf, extBuf);

		ofn.lpstrFileTitle = res;
		ofn.lpstrInitialDir = dirBuf;
	}
	else
	{
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrInitialDir = NULL;
	}




	// Display the Open dialog box. 
	
	if (GetOpenFileNameA(&ofn) == TRUE) {

		int len = strlen(ofn.lpstrFile);
		if (len > length) {
			return -1;
		}
		strcpy_s(buffer,length, ofn.lpstrFile);

		return 0;
	}

}

int edv_save_file(const char* lastOpenFile, const char* buffer, size_t length) {

	OPENFILENAMEA ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
	HWND hwnd = 0;              // owner window
	HANDLE hf;              // file handle

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;

	ofn.nMaxFileTitle = 0;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (lastOpenFile != NULL) {

		char dirBuf[256];
		char filenameBuf[256];
		char extBuf[256];

		//todo - error handling
		_splitpath_s(lastOpenFile, NULL, 0, dirBuf, 256, filenameBuf, 256, extBuf, 256);


		if ((strlen(filenameBuf) + strlen(extBuf)) > 256) {
			//file buffer too big
			return NULL;
		}

		wchar_t* res = strcat(filenameBuf, extBuf);

		ofn.lpstrFileTitle = res;
		ofn.lpstrInitialDir = dirBuf;
	}
	else
	{
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrInitialDir = NULL;
	}




	// Display the Open dialog box. 

	if (GetSaveFileNameA(&ofn) == TRUE) {

		int len = strlen(ofn.lpstrFile);
		if (len > length) {
			return -1;
		}
		strcpy_s(buffer, length, ofn.lpstrFile);

		return 0;
	}

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
#else //unsupported platform


#endif