#include "storage.h"
#include <stdio.h>


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


size_t index_offset = 0;//where to start showing the lines from
size_t line_start = 0;


size_t cursor_pos = 0;
int y_offset = 0;

void inc_cursor_y() {

    y_offset++;
}


void dec_cursor_y() {
    y_offset--;
}


void inc_cursor_x() {

    if (cursor_pos >= STR_END(str) - 1) {
        return;
    }

    cursor_pos++;
}
void dec_cursor_x() {


    if (cursor_pos == 0) {
        return;
    }

    cursor_pos--;
}



/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("editv", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        printf("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

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

    if (SDL_strlen(openFile) != 0) {
        SDL_snprintf(buf, 256, "%s - %s", appname, openFile);

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

    char* clip = SDL_GetClipboardText();

    size_t clip_size = SDL_strlen(clip);

    storage_insert(str, cursor_pos, clip, clip_size);
    cursor_pos += clip_size;
    SDL_free(clip);
}

void Undo() {

}

void SaveCallback(void* userdata, const char* const* filelist, int filter) {

    if (filelist[0] == NULL) {
        printf("Save Cancelled\n");
        return;
    }

    size_t len = SDL_strlen(filelist[0]);

    if (len > 256) {
        printf("File Path Too Long\n");
        return;
    }
    SDL_strlcpy(openFile, filelist[0], len+1);
    lastFilePath = openFile;

    

    SDL_IOStream *stream = SDL_IOFromFile(openFile, "w");

    SDL_WriteIO(stream, str->buffer, str->front_size * sizeof(char));
    SDL_WriteIO(stream, str->buffer + str->front_size + str->gap_size, str->buffer_size - str->front_size - str->gap_size * sizeof(char));



    SDL_CloseIO(stream);

    
    printf("Saved As: '%s'\n", openFile);

    UpdateTitle();
}

void OpenCallback(void* userdata, const char* const* filelist, int filter) {

    if (filelist[0] == NULL) {
        printf("Open Cancelled\n");
        return;
    }

    size_t len = SDL_strlen(filelist[0]);

    if (len > 256) {
        printf("File Path Too Long\n");
        return;
    }

    SDL_strlcpy(openFile, filelist[0], len+1);
    lastFilePath = openFile;



    SDL_IOStream* stream = SDL_IOFromFile(openFile, "r");

    if (stream == NULL) {
        printf("Failed to open file '%s'\n", openFile);
        return;
    }

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_END);

    long fsize = SDL_TellIO(stream);

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);

    
    const char* buf = SDL_malloc(sizeof(char) * fsize);
    if (buf == NULL) {
        printf("Failed to open file '%s'\n", openFile);
        return;
    }
    size_t count = SDL_ReadIO(stream, buf, fsize);

    if (count == 0) {
        printf("Failed to open file '%s'\n", openFile);
        SDL_CloseIO(stream);
        SDL_free(buf);
        return;
    }

    SDL_CloseIO(stream);

    Storage* s = storage_alloccopy(buf, count);

    SDL_free(buf);
    if (s == NULL) {
        printf("Failed to open file '%s'\n", openFile);
        
        return;
    }

    if (str) {
        storage_free(str);
    }


    printf("Buffer len = %zu, Gap len = %zu, Gap[0] = %zu\n", s->buffer_size, s->gap_size, s->front_size);

    storage_realloc(s);

    printf("Realloc:\nBuffer len = %zu, Gap len = %zu, Gap[0] = %zu\n", s->buffer_size, s->gap_size, s->front_size);



    str = s;

    printf("Opened: '%s'\n", openFile);
    UpdateTitle();
}

void Save() {

    const SDL_DialogFileFilter filters[] = {
        { "Text files",  "txt" },
        { "All files",   "*" }
    };

    SDL_ShowSaveFileDialog(SaveCallback, NULL, window, filters, 2, lastFilePath);
}

void Open() {

    const SDL_DialogFileFilter filters[] = {
    { "Text files",  "txt" },
    { "All files",   "*" }
    };

    SDL_ShowOpenFileDialog(OpenCallback, NULL, window, filters, 2, lastFilePath,0);
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
            storage_insert_c(str, '\n', cursor_pos);
            inc_cursor_x();
            //inc_cursor_y();
            //cursor_x = 0;
        }
        if (key == SDLK_BACKSPACE) {
            if (cursor_pos != 0) {
                dec_cursor_x();
                storage_remove(str, cursor_pos, 1);
                
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

        storage_insert(str, cursor_pos, text, SDL_strlen(text));

        for (size_t i = 0; i < SDL_strlen(text); i++)
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



void DrawMenu(float x, float y, size_t cursor_x, size_t cursor_y) {

    if (func_mode) {
        SDL_RenderDebugText(renderer, x, y, "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'   paste: 'v'");
    }
    else
    {
        const char buf[256];

        SDL_snprintf(buf, 256, "INSERT (%zu, %zu)", cursor_x, cursor_y);

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

    size_t cursor_x = 0;
    size_t cursor_y = line_start;


    //loop over every character in buffer
    while (index < STR_END(str)) {

        if (show_line_numbers) {

            SDL_snprintf(buf, buflen, "%d", curLine);

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


            //if (index == cursor_pos) {

            //    cursor_y = curLine;
            //    cursor_x = l;

            //}



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

    if (cursor_pos >= 0 && cursor_pos <= STR_END(str)) {

        if ((lines[line_count - 1].index + lines[line_count - 1].length) < cursor_pos) {

            cursor_y = line_count + line_start;
            cursor_x = 0;

        }
        else {

            for (size_t i = 0; i < line_count; i++)
            {
                if (cursor_pos >= lines[i].index && cursor_pos <= (lines[i].index + lines[i].length)) {
                    cursor_y = i + line_start;
                    cursor_x = cursor_pos - lines[i].index;
                    break;
                }
            }
        }

    }


    int new_y = cursor_y + y_offset;

    y_offset = 0;


    if (line_count > max_y && new_y >= line_count + line_start) { //off bottom of page
        //checking to make sure we only do this if theres more than 2 lines on page, which should never happen but id rather do a check than potentially jump to uninitialied memory
        if (line_count > 1) {
            index_offset = lines[1].index; //we have already cached the line index of the first line
            line_start++;

            cursor_pos = lines[line_count - 1].index + SDL_min(cursor_x, lines[line_count - 1].length);

            cursor_y = line_start + line_count;

            cursor_x = cursor_pos - lines[line_count - 1].index;
        }



    }
    else if (new_y < line_start) { //off top of page

        if (new_y > 0) {
            int in = index_offset; // points to the first character to be shown
            in -= 2; // move back and skip the newline

            int l = 0;
            //bounds checking
            if (in > 0) {
                //loops backward through the array until its reaches a newline
                while (storage_get(str, in) != '\n')
                {
                    in--;
                    l++;
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


            cursor_x = SDL_min(cursor_x, l);
            cursor_pos = in + cursor_x;

            cursor_y = line_start; //adjust the y pos to the start of the new page

            //cursor_x = cursor_pos - lines[line_count - 1].index;
        }


    }
    else { // on page

        if (new_y - line_start < line_count) {
            size_t i = SDL_clamp(new_y - line_start, 0, line_count);

            cursor_pos = lines[i].index + SDL_min(cursor_x, lines[i].length);

            cursor_y = i + line_start;
            cursor_x = cursor_pos - lines[i].index;
        }


    }



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



        cursor_pos = in;
        if (line_start != 0) {
            line_start--;
        }

        cursor_y = line_start; //adjust the y pos to the start of the new page

        cursor_x = cursor_pos - line_start;

    }

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


    DrawMenu(cw, ch, cursor_x, cursor_y);

}