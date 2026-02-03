#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "sfd.h"


#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>





static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

Storage* str;
//size_t Cursor = 0; //location in file of cursor

int w, h;

size_t cursor_x = 0;
size_t cursor_y = 0;


size_t index_offset = 0;//where to start showing the lines from
size_t line_start = 0;


size_t cached_cursor_pos = 0;

void inc_cursor_y() {

    if (cached_cursor_pos > STR_END(str)) {
        return;
    }

    cursor_y++;
}


void dec_cursor_y() {

    if (cached_cursor_pos == 0) {
        return;
    }

    if (cursor_y != 0) {
        cursor_y--;
    }
}


void inc_cursor_x() {

    if (cached_cursor_pos == STR_END(str)) {
        return;
    }

    float scale = 1;
    int x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale;
    int max_x = (int)((w - x * 2) * scale / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));



    if (cursor_x == max_x) {
        cursor_x = 0;
        inc_cursor_y();
    }
    else {
        cursor_x++;
    }
}
void dec_cursor_x() {


    if (cached_cursor_pos == 0) {
        return;
    }
    float scale = 1;
    int x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale;
    int max_x = (int)((w - x * 2) * scale / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));

    if (cursor_x == 0) {
        cursor_x = max_x;
        dec_cursor_y();
    }
    else {
        cursor_x--;
    }
}




size_t cursor_pos() {

    return cached_cursor_pos;

    float scale = 1;

    int x = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale;
    //int y = SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale;


    int max_x = (int)((w - x * 2) * scale / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));
    //int max_y = (int)((h - y * 2) * scale / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));

    int index = cursor_x + (cursor_y * max_x);

    return SDL_clamp(index,0, STR_END(str));
}


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("editv", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    //char* filename = "file.txt";

    //FILE* f = fopen(filename, "r");

    str = storage_alloc(0);
    if (str == NULL) {
        return SDL_APP_FAILURE;
    }

    //fclose(f);

    if (str == NULL) {
        return SDL_APP_FAILURE;
    }

    printf("\n\nBUFFER\n\n");

    printf("\n");



    //print_buffer(str);


    SDL_StartTextInput(window);

    return SDL_APP_CONTINUE;
}

wchar_t *lastFilePath = NULL;

void New() {
    storage_free(str);

    str = storage_alloc(0);
}

void Save() {
    sfd_Options opt = {
  .title = L"Save File",
  .filter_name = L"Text File",
  .filter = L"*.",
  .extension = L"txt",
  .path = lastFilePath,
    };

    const wchar_t* filename = sfd_save_dialog(&opt);

    if (filename) {
        lastFilePath = filename;

        FILE* f = _wfopen(filename, L"w");

        fwrite(str->buffer, sizeof(char), str->front_size, f);
        fwrite(str->buffer + str->front_size + str->gap_size, sizeof(char), str->buffer_size - str->front_size - str->gap_size, f);

        fclose(f);

        wprintf(L"Saved As: '%s'\n", filename);
    }
    else {
        printf("Save canceled\n");
    }
}

void Open() {
    sfd_Options opt = {
  .title = L"Open File",
  .filter_name = L"Text File",
  .filter = L"*.",
  .extension = L"txt",
  .path = lastFilePath,
    };

    const wchar_t* filename = sfd_open_dialog(&opt);

    if (filename) {
        lastFilePath = filename;


        if (str) {
            storage_free(str);
        }


        FILE* f = _wfopen(filename, L"r");

        str = storage_load(f);
        if (str == NULL) {
            wprintf(L"Failed to open file '%s'\n", filename);
            return NULL;
        }

        cursor_x = 0;
        cursor_y = 0;
        line_start = 0;
        index_offset = 0;

        fclose(f);

        wprintf(L"Opened: '%s'\n", filename);
    }
    else {
        printf("Open canceled\n");
    }
}


bool func_mode = false;

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        w = event->window.data1;
        h = event->window.data2;
    }

    if (event->type == SDL_EVENT_KEY_DOWN ) {
        SDL_Keycode key = event->key.key;
        if (key == SDLK_RETURN) {
            storage_insert_c(str, '\n', cursor_pos());
            inc_cursor_y();
            cursor_x = 0;
        }
        if (key == SDLK_BACKSPACE) {
            storage_remove(str, cursor_pos() - 1, 1);
            dec_cursor_x();
            
        }
        if (key == SDLK_LEFT) {
            dec_cursor_x();
        }
        if (key == SDLK_RIGHT) {
            inc_cursor_x();
        }
        if (key == SDLK_UP) {
            dec_cursor_y();
        }
        if (key == SDLK_DOWN) {
            inc_cursor_y();
        }

if (key == SDLK_LCTRL) {
    SDL_StopTextInput(window);
    func_mode = true;
}

if (func_mode) {
    if (key == SDLK_S) { //save
        Save();
    }
    if (key == SDLK_O) { //open
        Open();
    }

    if (key == SDLK_N) { //new
        New();
    }
    if (key == SDLK_Q) { //quit
        return SDL_APP_SUCCESS;
    }
}
    }

    if (event->type == SDL_EVENT_KEY_UP) {
        SDL_Keycode key = event->key.key;

        if (key == SDLK_LCTRL) {
            SDL_StartTextInput(window);
            func_mode = false;
        }
    }

    if (event->type == SDL_EVENT_TEXT_INPUT) {

        const char* text = event->text.text;

        storage_insert(str, cursor_pos(), text, strlen(text));
        for (size_t i = 0; i < strlen(text); i++)
        {
            inc_cursor_x();
        }
    }

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    w = 0;
    h = 0;
    float x, y, cw, ch;
    const float scale = 1.0f; //render scale of the characters - NOT WORKING
    const float line_gap = 2;

    //setup SDL for drawing
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);





    cw = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) * scale;
    ch = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) * scale;

    if (func_mode) {
        SDL_RenderDebugText(renderer, cw, ch, "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'");
    }
    else
    {
        SDL_RenderDebugText(renderer, cw, ch, "INSERT");
    }
    

    int xorigin = cw * 2;
    int yorigin = ch * 4;

    x = xorigin;
    y = yorigin;

    int max_x = (int)((w - x * 2) / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));
    int max_y = (int)((h - (y+line_gap) * 2) / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));





    int line = line_start; //line offset into array, we need this for proper cursor placement. cached so that we dont have to loop the entire array every time we render
    int index = index_offset; //start at the offset index
    int lines_drawn = 0; //used to check for the end of the page
    char buf[1024]; //stores a line before its rendered


    size_t lineIndexes[256];
    size_t lineIndexCount = 0;

    size_t cursorLineIndex = 0;
    size_t cyl = 0; //line the cursor is on
    size_t cxl = 0; //length of the line
    //cached_cursor_pos = str->front_size;



    //loop over every character in buffer
    while (index < STR_END(str)) { 

        //store current line index
        lineIndexes[lineIndexCount++] = index;


        //load line into buffer
        int l = 0;
        while (storage_get(str, index + l) != '\n' && l < max_x) { //loop until hit end of page or a newline

            buf[l] = storage_get(str, index + l);
            l++;
        }


        buf[l] = '\0'; //null terminate buffer


        //if the line that the cursor is one, store some extra information
        if (line == cursor_y) {

            if (l <= max_x && cursor_x > l) { //wrap lines properly
                inc_cursor_y();
                cursor_x = 0;
            }
            cursorLineIndex = index;
            cyl = line;
            cxl = l;
        }

        //use debug render text for now
        SDL_RenderDebugText(renderer, x, y, buf);


        //increment values for next loop
        y += ch;
        line++;
        index += l;

        lines_drawn++;

        if (lines_drawn > max_y) {
            break;
        }
    }

    //special casing for lines that end with a newline - ie a blank line at the bottom of the page
    if (cursor_y >= line) {
        if (storage_get(str, STR_END(str) == '\n')) { //last char is a newline
            cursorLineIndex = STR_END(str);
            cyl = line;
            cxl = 0;
        }
    }

    //text scrolling

    if (cursor_y < line_start) {//off the top of the page

        int in = index_offset; // points to the first character to be shown
        in -= 2; // move back and skip the newline


        //bounds checking
        if (in > 0) {
            //loops backward through the array until its reaches a newline
            while (storage_get(str, in) != '\n')
            {
                in--;
                if (in == 0) {
                    in--; //adjust so it lands on 0
                    break;
                }
            }
            in++;//move off the found newline
        }

        

        index_offset = in;
        if (line_start != 0) {
            line_start--;
        }

        cyl = line_start; //adjust the y pos to the start of the new page


    }

    if (cursor_y > line_start+max_y) {//off the bottom of the page

        //checking to make sure we only do this if theres more than 2 lines on page, which should never happen but id rather do a check than potentially jump to uninitialied memory
        if (lineIndexCount > 1) { 
            index_offset = lineIndexes[1]; //we have already cached the line index of the first line
            line_start++;
        }

    }

    //adjust the cursor pos to be a sane position
    cursor_x = min(cursor_x, cxl);//clamp the cursor x to the line end
    cursor_y = cyl;

    cached_cursor_pos = cursorLineIndex + cursor_x; //calculate the offset pos based on the calculated position in the array



    //draw cursor

    const Uint64 now = SDL_GetTicks();

    if (((int)(now % 1000) - 500) > 0) {
        SDL_FRect r;

        r.w = 2 * scale;
        r.h = 1 + SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;

        r.x = (xorigin + (cursor_x * cw));
        r.y = yorigin + ((cyl - line_start) * ch);//  y;// SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale + (cursor_y * (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2) * scale);

        SDL_RenderFillRect(renderer, &r);

    }




    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_StopTextInput(window);
    storage_free(str);
}