#include "storage.h"
#include <stdio.h>
#include "config.h"

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#include <SDL3_ttf/SDL_ttf.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

typedef struct
{
    size_t index; //index into the buffer that this line starts
    size_t length;
} line_t;


static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static TTF_Font* font = NULL;

extern unsigned char tiny_ttf[];
extern unsigned int tiny_ttf_len;

edv_config* cfg;
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

//const char const default_font[] = "assets\\CascadiaMono-Regular.otf";

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    cfg = load_config();

    /* Create the window */
    if (!SDL_CreateWindowAndRenderer("editv", 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        printf("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }




    
    /* Open the font */
    font = TTF_OpenFont(cfg->default_font, cfg->font_size);
    if (!font) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }



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

    Sint64 fsize = SDL_TellIO(stream);

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);

    
    char* buf = SDL_malloc(sizeof(char) * fsize);
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

    unsigned char *loadFrom = buf;

    unsigned char b0 = buf[0];
    unsigned char b1 = buf[1];
    unsigned char b2 = buf[2];

    if (b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) { //UTF BOM, strip
        loadFrom += 3;
        count -= 3;
        
    }

    Storage* s = storage_alloccopy(loadFrom, count);

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
        { "All files",   "*" },
        { "Text files",  "txt" }
    };

    SDL_ShowSaveFileDialog(SaveCallback, NULL, window, filters, 2, lastFilePath);
}

void Open() {

    const SDL_DialogFileFilter filters[] = {
        { "All files",   "*" },
        { "Text files",  "txt" }
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

    SDL_SetRenderDrawColor(renderer, cfg->background_color.r, cfg->background_color.g, cfg->background_color.b, cfg->background_color.a);
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
    unload_config(cfg);
}

void RenderTextAt(float x, float y, char* buf, SDL_Color color) {

    SDL_Surface* text;
    /* Create the text */
    text = TTF_RenderText_Blended(font, buf, 0, color);
    if (text) {
        texture = SDL_CreateTextureFromSurface(renderer, text);
        SDL_DestroySurface(text);

        if (texture != NULL) {
            SDL_FRect rect;
            rect.x = x;
            rect.y = y;

            rect.h = texture->h;
            rect.w = texture->w;


            SDL_RenderTexture(renderer, texture, NULL, &rect);

            SDL_DestroyTexture(texture);

        }
    }

}



void DrawMenu(float x, float y, size_t cursor_x, size_t cursor_y) {

    char* menu = "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'   paste: 'v'";
    char buf[256];

    char* textBuf;

    if (func_mode) {
        
        textBuf = menu;
        //SDL_RenderDebugText(renderer, x, y, "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'   paste: 'v'");
    }
    else
    {
        SDL_snprintf(buf, 256, "INSERT (%zu, %zu)", cursor_x, cursor_y);

        textBuf = buf;

        //SDL_RenderDebugText(renderer, x, y, buf);
    }


    SDL_Color color = { cfg->menu_color.r, cfg->menu_color.g, cfg->menu_color.b, SDL_ALPHA_OPAQUE };

    RenderTextAt(x, y, textBuf, color);
}

int show_line_numbers = 1;

void Draw() {
    w = 0;
    h = 0;
    float x, y, cw, ch;
    const float scale = 1.0f; //render scale of the characters - NOT WORKING

    
    const float line_gap = TTF_GetFontLineSkip(font);

    //setup SDL for drawing
    SDL_GetRenderOutputSize(renderer, &w, &h);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    cw = TTF_GetFontSize(font);
    ch = TTF_GetFontHeight(font);



    int xorigin = (int)cw * 2;
    if (show_line_numbers) {
        xorigin += (int)(cw * 4);
    }


    int yorigin = (int)ch * 4;

    x = (float)xorigin;
    y = (float)yorigin;

    //int max_x = (int)((w - xorigin) / (cw));
    int max_y = (int)((h - (yorigin + line_gap)) / (ch));





    int curLine = (int)line_start; //line offset into array, we need this for proper cursor placement. cached so that we dont have to loop the entire array every time we render
    int index = (int)index_offset; //start at the offset index



#define buflen 1024
    char buf[buflen]; //stores a line before its rendered


    line_t lines[256];
    lines[0].index = 0;
    lines[0].length = 0;
    size_t line_count = 0;



    size_t cursorLineIndex = 0;

    size_t cursor_x = 0;
    size_t cursor_y = line_start;


    //loop over every character in buffer
    while (index < STR_END(str)) {


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

            size_t measured;
            if (TTF_MeasureString(font, buf, l, w-xorigin-cw, NULL, &measured)) {
                if (measured < l) {
                    break;
                }
            }
        }

        buf[l] = '\0'; //null terminate buffer

        //use debug render text for now
        //SDL_RenderDebugText(renderer, x, y, buf);

        SDL_Color color = { cfg->text_color.r, cfg->text_color.g, cfg->text_color.b, SDL_ALPHA_OPAQUE };
        SDL_Surface* text;



        /* Create the text */
        text = TTF_RenderText_Blended(font, buf, 0, color);
        if (text) {
            texture = SDL_CreateTextureFromSurface(renderer, text);
            SDL_DestroySurface(text);

            if (texture != NULL) {
                SDL_FRect rect;
                rect.x = x;
                rect.y = y;

                rect.h = texture->h;
                rect.w = texture->w;


                SDL_RenderTexture(renderer, texture, NULL, &rect);

                SDL_DestroyTexture(texture);

            }
        }




        linedata.length = l;

        lines[line_count++] = linedata;

        //increment values for next loop
        y += line_gap;
        curLine++;

        if (line_count > max_y) {
            break;
        }
    }

    if (cursor_pos >= 0 && cursor_pos <= STR_END(str)) {
        if (line_count > 0) {
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


    }


    int new_y = (int)cursor_y + y_offset;

    new_y = SDL_max(new_y, 0);//keep this above zero to avoid weird wrapping errors

    y_offset = 0;


    if (line_count > max_y && new_y >= line_count + line_start) { //off bottom of page
        //checking to make sure we only do this if theres more than 2 lines on page, which should never happen but id rather do a check than potentially jump to uninitialied memory
        if (line_count > 2) {

            int in = (int)lines[line_count - 1].index + lines[line_count - 1].length; // points to the end of the last visible line
            in++;//skip over newline

            size_t i;
            for (i = 0; i < cursor_x; i++) //ensure that we dont accidentally skip multiple lines
            {
                if (storage_get(str, in + i) == '\n') {
                    break;
                }
            }

            index_offset = lines[1].index; //we have already cached the line index of the first line
            line_start++;

            cursor_pos = in + i;

            cursor_y = line_start + line_count-1;
            cursor_x = i;

        }



    }
    else if (new_y < line_start) { //off top of page

        if (new_y >= 0) {
            int in = (int)index_offset; // points to the first character to be shown
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
            else {
                in = 0;
            }



            index_offset = in;
            if (line_start != 0) {
                line_start--;
            }


            cursor_x = SDL_min(cursor_x, l);
            cursor_pos = in + cursor_x;

            cursor_y = line_start; //adjust the y pos to the start of the new page
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


    if (show_line_numbers) {

        for (size_t i = 0; i < line_count; i++)
        {
            SDL_snprintf(buf, buflen, "%zu", line_start+i);


            SDL_Color lineColor = { cfg->line_number_color.r,cfg->line_number_color.g, cfg->line_number_color.b, SDL_ALPHA_OPAQUE };
            if (cursor_y == line_start + i) {
                lineColor = (SDL_Color){ cfg->text_color.r, cfg->text_color.g, cfg->text_color.b, SDL_ALPHA_OPAQUE };
            }

            RenderTextAt(cw, yorigin + line_gap * i, buf, lineColor);
        }

    }


    //draw cursor

    const Uint64 now = SDL_GetTicks();

    if (((int)(now % 1000) - 500) > 0) {

        char* tt = SDL_malloc(cursor_x + 1);

        if (tt) {

            for (size_t i = 0; i < cursor_x; i++)
            {
                tt[i] = storage_get(str, cursor_pos - cursor_x + i);
            }
            tt[cursor_x] = '\0';

            TTF_Text* ttxt = TTF_CreateText(NULL, font, tt, 0);

            if (ttxt) {

                int tw, th;
                if (TTF_GetTextSize(ttxt, &tw, &th)) {

                    SDL_FRect r;

                    r.w = 2 * scale;
                    r.h = ch - 4 * scale;

                    r.x = (xorigin + (tw));
                    r.y = yorigin + ((cursor_y - line_start) * line_gap) + 2;//  y;// SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * 2 * scale + (cursor_y * (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2) * scale);

                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);


                    SDL_RenderFillRect(renderer, &r);

                }


                TTF_DestroyText(ttxt);
            }


            SDL_free(tt);
        }


    }


    DrawMenu(cw, ch, cursor_x, cursor_y);

}
