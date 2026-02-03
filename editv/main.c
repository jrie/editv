#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "interface.h"


#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#define min(a,b) (((a) < (b)) ? (a) : (b))

typedef struct
{
    size_t index; //index into the buffer that this line starts
    size_t length;
} line_t;


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
        cursor_x = 1;
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
}


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("editv", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    edv_init(window);

    //char* filename = "file.txt";

    //FILE* f = fopen(filename, "r");

    str = storage_alloc(1);
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

char *lastFilePath = NULL;

char openFile[256];

const char* const appname = "editv";

void UpdateTitle() {

    char buf[256];

    if (strlen(openFile) != 0) {
        snprintf(buf, 256, "%s - %s", appname, openFile);

        SDL_SetWindowTitle(window, buf); 
    }
    else
    {
        SDL_SetWindowTitle(window, appname);
    }


}

void New() {
    storage_free(str);

    str = storage_alloc(1);

    openFile[0] = 0;
    UpdateTitle();
}

void Paste() {
    char* clip;
    size_t clip_size = edv_clipboard(&clip);

    storage_insert(str, cursor_pos(), clip, clip_size);
    cursor_x += clip_size-1;

    free(clip);
}

void Undo() {

}

void Save() {

    int found = edv_save_file(lastFilePath, openFile, 256);

    if (found == 0) {
        lastFilePath = openFile;

        FILE* f = fopen(openFile, "w");

        fwrite(str->buffer, sizeof(char), str->front_size, f);
        fwrite(str->buffer + str->front_size + str->gap_size, sizeof(char), str->buffer_size - str->front_size - str->gap_size, f);

        fclose(f);

        printf("Saved As: '%s'\n", openFile);
    }
    else {
        printf("Save canceled\n");
    }
}

void Open() {

    int found = edv_open_file(lastFilePath, openFile, 256);

    if (found == 0) {

        lastFilePath = openFile;

        
        UpdateTitle();


        FILE* f = fopen(openFile, "r");

        Storage *tmp = storage_load(f);
        if (tmp == NULL) {
            printf("Failed to open file '%s'\n", openFile);
            return;
        }

        if (str) {
            storage_free(str);
        }
        str = tmp;

        cursor_x = 0;
        cursor_y = 0;
        line_start = 0;
        index_offset = 0;

        fclose(f);

        printf("Opened: '%s'\n", openFile);
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
            if (cursor_pos() != 0) {
                dec_cursor_x();
                storage_remove(str, cursor_pos() - 1, 1);
                
            }

            
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
    if (key == SDLK_Z) { //undo
        Undo();
    }
    if (key == SDLK_V) { //paste
        Paste();
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

void Draw();

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    Draw();

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}


/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_StopTextInput(window);
    storage_free(str);
}



void DrawMenu(float x, float y) {

    if (func_mode) {
        SDL_RenderDebugText(renderer, x, y, "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'   paste: 'v'");
    }
    else
    {
        const char buf[256];

        snprintf(buf, 256, "INSERT (%zu, %zu)", cursor_x, cursor_y);

        SDL_RenderDebugText(renderer, x, y, buf);
    }
}

int show_line_numbers = 1;

void Draw() {
    w = 0;
    h = 0;
    float x, y, cw, ch;
    const float scale = 1.0f; //render scale of the characters - NOT WORKING
    const float line_gap = 2;

    //setup SDL for drawing
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    cw = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)*scale;
    ch = (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)*scale;

    int xorigin = cw * 2;
    if (show_line_numbers) {
        xorigin += cw * 4;
    }


    int yorigin = ch * 4;

    x = xorigin;
    y = yorigin;

    int max_x = (int)((w - x * 2) / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));
    int max_y = (int)((h - (y + line_gap) * 2) / (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE));





    int curLine = line_start; //line offset into array, we need this for proper cursor placement. cached so that we dont have to loop the entire array every time we render
    int index = index_offset; //start at the offset index



#define buflen 1024
    char buf[buflen]; //stores a line before its rendered


    line_t lines[256];
    size_t line_count = 0;



    size_t cursorLineIndex = 0;


    //loop over every character in buffer
    while (index < STR_END(str)) {

        if (show_line_numbers) {

            snprintf(buf, buflen, "%d", curLine);

            SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255);

            SDL_RenderDebugText(renderer, cw, y, buf);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }


        //store current line index

        line_t linedata;
        linedata.index = index;


        //load line into buffer
        int l = 0;

        while (l < buflen && index < STR_END(str))
        {
            char c = storage_get(str, index);

            if (c == '\n') { //hit a newline
                index++; //move index over the newline
                break;
            }



            //copy char and increase line length
            buf[l++] = c;



            index++; //increment the index to the next char

            if (l == max_x) { //hit end of page, so wrap
                //index++;
                break;
            }
        }

        buf[l] = '\0'; //null terminate buffer




        //use debug render text for now
        SDL_RenderDebugText(renderer, x, y, buf);


        linedata.length = l;

        lines[line_count++] = linedata;

        //increment values for next loop
        y += ch;
        curLine++;

        if (line_count > max_y) {
            break;
        }
    }



    //find cursor position

    if (cursor_y < line_start) {

        //scroll up
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

            cursor_y = line_start; //adjust the y pos to the start of the new page

        }

    }
    else if (cursor_y < line_start + line_count) {
        line_t cursor_line = lines[cursor_y - line_start]; //get the line the cursor is on

        //adjust the cursor pos to be a sane position
        cursor_x = min(cursor_x, cursor_line.length);//clamp the cursor x to the line end
        cursorLineIndex = cursor_line.index;
    }
    else if (cursor_y >= line_start + line_count) { //past the end of the line buffer


        //special casing for lines that end with a newline - ie a blank line at the bottom of the page

        char c = storage_get(str, STR_END(str));

        if (storage_get(str, STR_END(str)) == '\n') { //last char is a newline or eof

            cursor_y = line_start + line_count;
            cursor_x = 0;
            cursorLineIndex = STR_END(str);
        }
        else //past the end line when it shouldnt be
        {

            //scroll down
            if (cursor_y > line_start + max_y) {//off the bottom of the page

                //checking to make sure we only do this if theres more than 2 lines on page, which should never happen but id rather do a check than potentially jump to uninitialied memory
                if (line_count > 1) {
                    index_offset = lines[1].index; //we have already cached the line index of the first line
                    line_start++;
                }

            }
            else if (line_count != 0) {

                line_t cursor_line = lines[line_count - 1]; //get the line the cursor is on

                //adjust the cursor pos to be a sane position

                cursor_y = line_start + line_count - 1;
                cursor_x = min(cursor_x, cursor_line.length);//clamp the cursor x to the line end
                cursorLineIndex = cursor_line.index;
            }

        }
    }


    cached_cursor_pos = cursorLineIndex + cursor_x; //calculate the offset pos based on the calculated position in the array


    //draw cursor

    const Uint64 now = SDL_GetTicks();

    if (((int)(now % 1000) - 500) > 0) {
        SDL_FRect r;

        r.w = 2 * scale;
        r.h = 1 + SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * scale;

        r.x = (xorigin + (cursor_x * cw));
        r.y = yorigin + ((cursor_y - line_start) * ch);//  y;// SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale + (cursor_y * (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2) * scale);

        SDL_RenderFillRect(renderer, &r);

    }


    DrawMenu(cw, ch);

}