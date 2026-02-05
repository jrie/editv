#include "storage.h"
#include <stdio.h>
#include "config.h"

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


#include <SDL3_ttf/SDL_ttf.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

#define EDV_VERSION_MAJOR 0
#define EDV_VERSION_MINOR 1
#define EDV_VERSION_PATCH 5

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)

#define VERSION STRINGIFY(EDV_VERSION_MAJOR)"."STRINGIFY(EDV_VERSION_MINOR)"."STRINGIFY(EDV_VERSION_PATCH)



const char* const appname = "editv "VERSION;

typedef struct
{
    size_t number;
    size_t index; //index into the buffer that this line starts
    size_t length;
} line_t;


static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static TTF_Font* font = NULL;


bool func_mode = false;
bool shift_down = false;
bool unsaved_changes = false;


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


int show_line_numbers = 1;
int wrap_lines = 1;
int cache_lines = 0; //CURRENTLY BROKEN, DO NOT ENABLE

typedef struct {
    line_t line;
    SDL_Texture* tex;
    int filled;
}cached_line;

cached_line cached_lines[256];
size_t cached_line_count = 0;


void inc_cursor_y() {

    y_offset++;
}


void dec_cursor_y() {
    y_offset--;
}


void inc_cursor_x() {

    if (cursor_pos >= STR_END(str) ) {
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

char* lastFilePath = NULL;

char openFile[256];


void UpdateTitle() {

    char buf[256];

    printf("Open File is '%s' with length %zu\n", openFile, strlen(openFile));

    if (SDL_strlen(openFile) != 0) {
        SDL_snprintf(buf, 256, "%s - %s", appname, openFile);

        SDL_SetWindowTitle(window, buf);
    }
    else
    {
        SDL_SetWindowTitle(window, appname);
    }


}

//#define KRED  "\x1B[31m"
//returns 1 if continue, 0 if close
int ParseArgs(int argc, char* argv[]) {


    //flags specified
    int c = 0; //create new file

    for (size_t i = 1; i < argc; i++) //loop over args looking for flags
    {
        size_t len = strlen(argv[i]);
        if (len == 0) continue;

        if (argv[i][0] == '-') {


            if (len < 2) goto invalid_args;//make sure theres enough space to not cause overflow

            //double dash args

            if (!strcmp(argv[i], "--version"))
            {
                SDL_Log("editv %s\n", VERSION);
                SDL_Log("Copyright (C) 2026 nimrag\n");
                return 0;
            }
            else if (!strcmp(argv[i], "--help"))
            {
                SDL_Log("Usage\n\n");
                SDL_Log("\teditv\n\n");

                SDL_Log("Opens with no open file.\n\n");

                SDL_Log("\teditv [OPTIONS] <path>\n\n");

                SDL_Log("Opens with the file at <path> loaded.\n\n");

                SDL_Log("Options\n\n");
                SDL_Log("\t-write, -w <path>       = Write/create a new file at <path>\n");

                SDL_Log("\n");

                SDL_Log("\t--version               = Print current version and exit\n");
                SDL_Log("\t--help                  = Print usage information and exit\n");
                return 0;
            }

            //single dash args
            else if (!strcmp(argv[i], "-write") || !strcmp(argv[i], "-w")) { //write flag
                w = 1;
            }
            else { //unknown flag
                SDL_Log("Unknown Flag %s\n", argv[i]);
                goto invalid_args;
            }

        }
    }

    for (size_t i = 1; i < argc; i++)
    {
        size_t len = strlen(argv[i]);
        if (len == 0) continue;

        if (argv[i][0] == '-') {
            //flags arlready parsed
            continue;
        }
        else { //try open a file

            Storage* s = storage_fromfile(argv[i],w);

            if (s == NULL) {
                return 0;
            }

            SDL_strlcpy(openFile, argv[i], len+1);
            lastFilePath = openFile;

            //this should NEVER even have a chance to happen but better to be safe than accidentally leak memory
            if (str) {
                storage_free(str);
            }


            printf("Buffer len = %zu, Gap len = %zu, Gap[0] = %zu\n", s->buffer_size, s->gap_size, s->front_size);

            str = s;


            return 1;
        }


    }

    return 1; //normal operation, exit

invalid_args:
    SDL_Log("Invalid Arguments: ");
    for (size_t i = 1; i < argc-1; i++)
    {
        SDL_Log("%s, ", argv[i]);
    }
    SDL_Log("%s", argv[argc-1]);
    return 0;
}

//const char const default_font[] = "assets\\CascadiaMono-Regular.otf";

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (argc > 1) {
        if (!ParseArgs(argc, argv)) {
            return SDL_APP_FAILURE;
        }
    }
    /* Create the window */
    if (!SDL_CreateWindowAndRenderer(appname, 800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        printf("Couldn't create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderVSync(renderer, 1);



    if (!TTF_Init()) {
        SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    //call in case a file was loaded from the cmdline
    UpdateTitle();

    cfg = load_config();




    
    /* Open the font */
    font = TTF_OpenFont(cfg->default_font, cfg->font_size);
    if (!font) {
        SDL_Log("Couldn't open font: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    if (str == NULL) { //wasnt already allocated by cmd arguments
        str = storage_alloc(0);
        if (str == NULL) {
            return SDL_APP_FAILURE;
        }
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



void New() {
    storage_free(str);

    str = storage_alloc(0);

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

#ifdef _WIN32
#define SAVE_MODE "w,ccs=UTF-8" //linux doesnt seem to like this flag but on windows without it it causes the file to be interpreted as UTF16 by some programs
#else
#define SAVE_MODE "w"
#endif


void SaveTo(const char* path) {

    SDL_IOStream* stream = SDL_IOFromFile(path, SAVE_MODE);
    if (stream == NULL) {
        SDL_Log("Invalid save path: '%s'\n",path);
        return;
    }

    const char UTF_BOM[] = { 0xEF ,0xBB,0xBF };


    //SDL_WriteIO(stream, UTF_BOM, 3);

    SDL_WriteIO(stream, str->buffer, str->front_size * sizeof(char));
    SDL_WriteIO(stream, str->buffer + str->front_size + str->gap_size, str->buffer_size - str->front_size - str->gap_size * sizeof(char));

    SDL_CloseIO(stream);

    unsaved_changes = false;
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

    
    SaveTo(openFile);


    
    printf("Saved As: '%s'\n", openFile);

    UpdateTitle();
}


void OpenCallback(void* userdata, const char* const* filelist, int filter) {

    if (filelist[0] == NULL) {
        printf("Open Cancelled\n");
        return;
    }

    Storage *s = storage_fromfile(filelist[0], 0);

    if (s == NULL) {
        return;
    }

    SDL_strlcpy(openFile, filelist[0], strlen(filelist[0])+1);
    lastFilePath = openFile;


    if (str) {
        storage_free(str);
    }


    printf("Buffer len = %zu, Gap len = %zu, Gap[0] = %zu\n", s->buffer_size, s->gap_size, s->front_size);

    str = s;

    UpdateTitle();
}

void Save() {

    if (strlen(openFile) != 0) {
        SaveTo(openFile);
        return;
    }

    const SDL_DialogFileFilter filters[] = {
        { "Text files",  "txt" },
        { "All files",   "*" },
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
            unsaved_changes = true;
        }
        if (key == SDLK_BACKSPACE) {
            if (cursor_pos != 0) {
                dec_cursor_x();
                storage_remove(str, cursor_pos, 1);
                unsaved_changes = true;
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
        if (key == SDLK_LSHIFT) {
            shift_down = true;
        }

        if (func_mode) {

            if (key == SDLK_S) { //save
                if (shift_down) {
                    memset(openFile, 0, strlen(openFile)); //clear open file
                    //UpdateTitle();
                }
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

        if (key == SDLK_LSHIFT) {
            shift_down = false;
        }
    }

    if (event->type == SDL_EVENT_TEXT_INPUT) {

        const char* text = event->text.text;

        storage_insert(str, cursor_pos, text, SDL_strlen(text));
        unsaved_changes = true;

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
    if (str) storage_free(str);
    
    if (cfg) unload_config(cfg);
}

void RenderTextAt(float x, float y, char* buf, SDL_Color color) {

    SDL_Surface* text;
    /* Create the text */
    text = TTF_RenderText_Blended(font, buf, 0, color);
    if (text) {
        texture = SDL_CreateTextureFromSurface(renderer, text);
        

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
    //always ensure that no matter whar we free this so we dont leak
    SDL_DestroySurface(text);

}

char* menu_options[] = {
    "open : 'o'",
    "save : 's'",
    "new : 'n'",
    "quit : 'q'",
    "paste : 'v'"
};

char* alt_menu_options[] = {
    "save as : 's'",
};

void DrawMenu(float x, float y, size_t cursor_x, size_t cursor_y) {

    //char* menu = "FUNC   open: 'o'   save: 's'   new: 'n'   quit: 'q'   paste: 'v'";
    char buf[256];

    char* textBuf;

    if (func_mode) {
        
        if (shift_down) {
            int stride = SDL_snprintf(buf, 256, "FUNC(alt)");

            for (size_t i = 0; i < sizeof(alt_menu_options) / sizeof(alt_menu_options[0]); i++)
            {
                stride += SDL_snprintf(buf + stride, 256 - stride, "   %s", alt_menu_options[i]);
            }

        }
        else {
            int stride = SDL_snprintf(buf, 256, "FUNC");

            for (size_t i = 0; i < sizeof(menu_options) / sizeof(menu_options[0]); i++)
            {
                stride += SDL_snprintf(buf + stride, 256 - stride, "   %s",menu_options[i]);
            }
        }



        textBuf = buf;
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




SDL_Texture* CacheLine(line_t line, char* buf, SDL_Color color) {

    SDL_Surface* text;
    /* Create the text */
    text = TTF_RenderText_Blended(font, buf, 0, color);
    if (text) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text);
        cached_lines[line.number - line_start] = (cached_line){ line,texture,1 };
        //always ensure that no matter whar we free this so we dont leak
        SDL_DestroySurface(text);
        return texture;
    }
    //always ensure that no matter whar we free this so we dont leak
    SDL_DestroySurface(text);
    return NULL;
}

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


    if (unsaved_changes) {
        char astk[] = "*";
        SDL_Color color = { cfg->text_color.r, cfg->text_color.g, cfg->text_color.b, SDL_ALPHA_OPAQUE };
        RenderTextAt(w-ch, ch, astk, color);
    }


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
    while (index <= STR_END(str)) {


        //store current line index

        line_t linedata;

        //current line is not cached
        if (!cache_lines || (!cached_lines[curLine - line_start].filled || cached_lines[curLine - line_start].line.number != curLine)) {
            if (cached_lines[curLine - line_start].tex != NULL) {
                SDL_DestroyTexture(cached_lines[curLine - line_start].tex);//so we dont leak
            }


            linedata.index = index;
            linedata.number = curLine;

            //load line into buffer
            int l = 0;

            while (l < buflen && index <= STR_END(str))
            {
                char c = storage_get(str, index);

                if (c == '\n') { //hit a newline
                    index++; //move index over the newline
                    break;
                }

                //copy char and increase line length
                buf[l++] = c;

                index++; //increment the index to the next char

                if (wrap_lines) {
                    size_t measured;
                    if (TTF_MeasureString(font, buf, l, w - xorigin - cw, NULL, &measured)) {
                        if (measured < l) {
                            curLine--;
                            break;
                        }
                    }
                }

            }

            buf[l] = '\0'; //null terminate buffer


            linedata.length = l;
            SDL_Color color = { cfg->text_color.r, cfg->text_color.g, cfg->text_color.b, SDL_ALPHA_OPAQUE };

            SDL_Texture* tex = CacheLine(linedata, buf, color);

            if (tex != NULL) {
                SDL_FRect rect;
                rect.x = x;
                rect.y = y;

                rect.h = tex->h;
                rect.w = tex->w;


                SDL_RenderTexture(renderer, tex, NULL, &rect);



            }
        }
        else
        {
            cached_line ln = cached_lines[curLine - line_start];
            linedata = ln.line;

            index += ln.line.length;

            SDL_Texture* tex = ln.tex;

            if (tex != NULL) {
                SDL_FRect rect;
                rect.x = x;
                rect.y = y;

                rect.h = tex->h;
                rect.w = tex->w;


                SDL_RenderTexture(renderer, tex, NULL, &rect);



            }
        }



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
        int last = -1;
        for (size_t i = 0; i < line_count; i++)
        {
            if (lines[i].number != last) { //dont draw multiple line numbers for wrapped lines. perhaps make this an option in the future
                SDL_snprintf(buf, buflen, "%zu", lines[i].number);

                last = lines[i].number;


                SDL_Color lineColor = { cfg->line_number_color.r,cfg->line_number_color.g, cfg->line_number_color.b, SDL_ALPHA_OPAQUE };
                if (cursor_y == line_start + i) {
                    lineColor = (SDL_Color){ cfg->text_color.r, cfg->text_color.g, cfg->text_color.b, SDL_ALPHA_OPAQUE };
                }

                RenderTextAt(cw, yorigin + line_gap * i, buf, lineColor);
            }

        }

    }


    if(cache_lines) cached_lines[cursor_y - line_start].filled = 0; //dont cache the line that the cursor is on

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
