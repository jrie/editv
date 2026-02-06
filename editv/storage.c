#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_iostream.h"
#include "storage.h"
#include <stdio.h>


static void strcmd_trim(Storage* str) {

    StorageCommand* first = str->first_undo;
    if (first == NULL) return;

    str->first_undo = first->next;

    if (str->StoredUndos == 1) {
        str->last_undo = NULL;
    }


    SDL_free(first->data);
    SDL_free(first);

    str->StoredUndos--;
}

static void strcmd_pop(Storage* str) {

    StorageCommand* top = str->last_undo;
    if (top == NULL) return;


    str->last_undo = top->prev;

    if (str->StoredUndos == 1) {
        str->first_undo = NULL;
    }

    SDL_free(top->data);
    SDL_free(top);

    str->StoredUndos--;
}

static void strcmd_pop_redo(Storage* str) {

    StorageCommand* top = str->last_redo;
    if (top == NULL) return;


    str->last_redo = top->prev;

    SDL_free(top->data);
    SDL_free(top);
}



void strcmd_clear_redos(Storage* str) {

    while (str->last_redo != NULL) {
        strcmd_pop_redo(str);
    }
}


static void strcmd_add(Storage *str, StorageCommandType type, const char* data, size_t index, size_t length, StorageSaveMode savemode) {

    if (savemode == STR_NONE) return;

    StorageCommand *cmd = SDL_malloc(sizeof(StorageCommand));

    char* txt = SDL_malloc(length * sizeof(char));

    SDL_memcpy(txt, data, length * sizeof(char));

    cmd->data = txt;
    cmd->cmd = type;
    cmd->next = NULL;
    cmd->index = index;
    cmd->length = length;

    if (savemode & STR_UNDO) {
        cmd->prev = str->last_undo;

        if (str->first_undo == NULL) {
            str->first_undo = cmd;
        }
        else
        {
            str->last_undo->next = cmd;

        }

        str->last_undo = cmd;

        str->StoredUndos++;

        if (str->StoredUndos > COMMAND_HISTORY_MAX) {
            strcmd_trim(str);
        }


        if (!(savemode & STR_REDO)) {
            strcmd_clear_redos(str);
        }
    }
    else if (savemode & STR_REDO) {
        cmd->prev = str->last_redo;

        str->last_redo = cmd;
    }


}


void storage_free(Storage* str){


    while (str->StoredUndos != 0)
    {
        strcmd_pop(str); //free stored command memory
    }

    SDL_free(str->buffer);
    SDL_free(str);
}

Storage* storage_alloc(size_t size){

    Storage *s = SDL_malloc(sizeof(Storage));

    //if (size == 0) {

    //    printf("Cannot create buffer with size of 0\n");
    //    return NULL;
    //}

    if (s == NULL) {
        printf("Failed to create storage buffer\n");
        return NULL;
    }


    s->front_size = size; //because the front buffer doesnt like being zero and will cause a weird initial state that has to be reset by intserting a newline
    s->gap_size = BUFFER_GAP_SIZE;


    s->buffer_size = size + BUFFER_GAP_SIZE;

    char* tmp = SDL_malloc(s->buffer_size+1);
    if (tmp == NULL) {
        return NULL;
    }

    s->buffer = tmp;

    s->buffer[s->buffer_size] = 0;

    s->first_undo = NULL;
    s->last_undo = NULL;
    s->last_redo = NULL;
    s->StoredUndos = 0;
    //s->buffer[s->front_size] = '\0';
    return s;

}

Storage* storage_alloccopy(const char* string, size_t size) {

    Storage* s = storage_alloc(size);
    
    SDL_memcpy(s->buffer, string, size);


    return s;

}

Storage* storage_fromfile(const char* path, int create) {

    size_t len = SDL_strlen(path);
    char* filemode;
    if (create) {
        filemode = "w";
    }
    else {
        filemode = "r";
    }

    SDL_IOStream* stream = SDL_IOFromFile(path, filemode);

    if (stream == NULL) {
        goto failed_open;
    }

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_END);

    Sint64 fsize = SDL_TellIO(stream);

    SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);

    if (fsize == -1) {
        SDL_CloseIO(stream);
        goto failed_open;
    }

    char* buf = SDL_malloc(sizeof(char) * fsize + 1); //minimum of 1 byte allocated
    if (buf == NULL) {
        goto failed_open;
    }
    size_t count = SDL_ReadIO(stream, buf, fsize);

    if (count == 0) {
        //construct a 'file' buffer that is 1 element long and only contains an EOF
        buf[0] = '\0';
        count = 1;
    }

    SDL_CloseIO(stream);

    unsigned char* loadFrom = buf;

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
        goto failed_open;
    }

    printf("Opened file at '%s'\n", path);
    return s;

failed_open:
    printf("Failed to open file '%s'\n", path);
    return NULL;
}

//index storage ignoring the gap
char storage_get(Storage *str, size_t index){

    if(index >= str->front_size){
        index += str->gap_size;
    }

    if(index >= str->buffer_size){
        return 0;
    }

    return str->buffer[index];
}


char* storage_ptr(Storage *str, size_t index){

    if(index >= str->front_size){
        index += str->gap_size;
    }

    if(index >= str->buffer_size){
        return 0;
    }

    return str->buffer + index;
}

void storage_move_gap(Storage *str, size_t pos){

    if (pos < 0 || pos > str->buffer_size) {
        return;
    }

    long long bytesToMove = str->front_size - pos; //encodes both amount and direction to shift bytes


    char* gap = (str->buffer + str->front_size); // points towards start of gap
    char* back = (gap+str->gap_size); // points towards start of back


    if(bytesToMove > 0){ //moving into front buffer
        //take some bytes off the front buffer and move them into the back buffer
        memmove(back - bytesToMove, gap - bytesToMove,bytesToMove);
    }
    else{ //moving into back buffer
        memmove(gap, back,-bytesToMove);
    }


    str->front_size = str->front_size - bytesToMove;
}

int storage_grow(Storage *str){

    printf("realloced\n");
    storage_move_gap(str, STR_END(str)); //move gap to front of storage
    
    str->buffer_size += BUFFER_GAP_SIZE;

    char* tmp = NULL;
    tmp = SDL_realloc(str->buffer, str->buffer_size);
    if(tmp == NULL){
        return 0;
    }
    str->buffer = tmp;


    str->gap_size += BUFFER_GAP_SIZE;

    return -1;
}

int storage_realloc(Storage *str){

    printf("realloced\n");
    storage_move_gap(str, STR_END(str)); //move gap to front of storage

    str->buffer_size -= str->gap_size - BUFFER_GAP_SIZE;

    char* tmp = NULL;
    tmp = SDL_realloc(str->buffer, str->buffer_size);
    if (tmp == NULL) {
        return 0;
    }
    str->buffer = tmp;

    str->gap_size = BUFFER_GAP_SIZE;

    return 1;
}



int storage_insert_c(Storage * str, char c, size_t pos, StorageSaveMode savemode){

    
    if(pos < 0 || pos > STR_END(str)){
        return 0;
    }


    if(str->gap_size == 0){
        storage_grow(str);
    }

    if(str->front_size != pos){
        storage_move_gap(str,pos);
    }

    str->buffer[str->front_size] = c;

    str->front_size++;
    str->gap_size--;

    strcmd_add(str, STORAGE_INSERT, &c, pos, 1, savemode);

    return 1;
}

int storage_insert(Storage * str, size_t pos,const char *s, size_t length, StorageSaveMode savemode){

    if(s[length-1] == 0){ // strip null terminator
        length--;
    }



    if(pos < 0 || pos > STR_END(str)){
        return 0;
    }


    while(str->gap_size < length){ //add more bytes until gap big enough to insert
        storage_grow(str);
    }


    if(str->front_size != pos){
        storage_move_gap(str,pos);
    }


    SDL_memcpy((str->buffer+str->front_size),s,length);

    str->front_size+=length;
    str->gap_size-=length;

    strcmd_add(str, STORAGE_INSERT, s, pos, length, savemode);

    return 1;
}


int storage_remove(Storage * str, size_t pos, size_t count, StorageSaveMode savemode){

    if(pos+count > STR_END(str)){
        return 0;
    }

    if(str->front_size != pos+count){
        storage_move_gap(str,pos+count);
    }


    //dont actually have to do any allocating, just mark the area as free
    str->front_size-=count;
    str->gap_size+=count;


    strcmd_add(str, STORAGE_REMOVE, (str->buffer + str->front_size), pos, count, savemode);


    storage_move_gap(str,pos);

    if(str->gap_size > BUFFER_GAP_SIZE){
        storage_realloc(str);
    }




    return 1;
    
}


int storage_overwrite(Storage * str, size_t pos, char *s, size_t length){

    if(pos+length > STR_END(str)){
        return 0;
    }

    if(str->front_size != pos+length+1){
        storage_move_gap(str,pos+length+1);
    }


    SDL_memcpy((str->buffer+str->front_size-length-1),s,length);

    return 1;
}


//fills buffer with index of matching strings
//returns num found
int storage_match(Storage * str, int *buffer, size_t max, char *expr){

    int buf_index = 0;


    size_t len = SDL_strlen(expr);

    for (int i = 0; i < STR_END(str); i++)
    {
        for (int j = 0; j < len; j++)
        {
            if(expr[j] != storage_get(str,i+j)){
                goto fail;
            }
        }
        //succeeds here
        buffer[buf_index] = i;
        buf_index++;
        if(buf_index > max){
            return buf_index;
        }

        fail:
        continue;
    }
    
    return buf_index;

}

size_t storage_nextline(Storage* str, size_t index) {

    char ch = 0;
    while (ch = storage_get(str,index++) && ch != '\n')
    {

    }

    return index;

}



//on success returns the index of the undone command, on failure returns -1
int storage_undo(Storage* str) {

    if (str->StoredUndos == 0) {
        return -1;
    }


    StorageCommand* cmd = str->last_undo;
    if (cmd == NULL) return -1;

    switch (cmd->cmd)
    {
    case STORAGE_INSERT: //data was inserted that has to be removed
        storage_remove(str, cmd->index, cmd->length, STR_NONE);
        break;
    case STORAGE_REMOVE: //data was removed that has to be readded
        storage_insert(str, cmd->index,cmd->data, cmd->length, STR_NONE);
        break;

    default:
        break;
    }

    strcmd_add(str, cmd->cmd, cmd->data, cmd->index, cmd->length, STR_REDO);

    int index = cmd->index;
    strcmd_pop(str);
    return index;
}

//on success returns the index of the undone command, on failure returns -1
int storage_redo(Storage* str) {

    StorageCommand* cmd = str->last_redo;
    if (cmd == NULL) return -1;

    switch (cmd->cmd)
    {
    case STORAGE_INSERT:
        storage_insert(str, cmd->index, cmd->data, cmd->length, STR_NONE);
        break;
    case STORAGE_REMOVE:
        storage_remove(str, cmd->index, cmd->length, STR_NONE);

        break;

    default:
        break;
    }

    strcmd_add(str, cmd->cmd, cmd->data, cmd->index, cmd->length, STR_REUNDO);

    int index = cmd->index + cmd->length;
    strcmd_pop_redo(str);
    return index;
}


//currently doesnt save changes in undo
void storage_replaceall(Storage *str, char *target, char* s){

    int names[512];

    int c = storage_match(str,names,sizeof(names) / sizeof(names[0]),target);

    size_t targetlength = SDL_strlen(target);

    size_t slength = SDL_strlen(s);

    for (int i = c; i > 0; i--)
    {
        storage_remove(str,names[i-1],targetlength, STR_NONE);
        storage_insert(str,names[i-1],s,slength, STR_NONE);
    }
}



void print_buffer(Storage * str){
    //printf("\'%.*s%.*s\'\n", str->front_idx + 1, str->buffer,str->buffer_size - str->gap_size - str->front_idx + 1 ,(str->buffer + str->front_idx + 1 + str->gap_size));

    int i = 0;
    char ch;
    while (ch = storage_get(str,i++))
    {
        putchar(ch);
    }
}

void print_buffer_separate(Storage * str){
    
    printf("\nFRONT: \n\n");
    for (size_t i = 0; i < str->front_size; i++)
    {
        putchar(str->buffer[i]);
    }

    printf("\nBACK: \n\n");
    for (size_t i = str->front_size+str->gap_size; i <= str->buffer_size; i++)
    {
        putchar(str->buffer[i]);
    }

}

void print_lines(Storage *str){
    size_t i = 0;
    int l = 1;
    while (i < str->buffer_size)
    {
        printf("%d: ",l++);

        while (i < str->buffer_size && str->buffer[i] != '\n')
        {

            putchar(str->buffer[i]);

            //skip gap
            if(i == str->front_size-1){ 
                i += str->gap_size;
            }
            else{
                i++;
            }
        }
        printf("\n");
        i++;

    }
    
}

void storage_wipe_gap(Storage *str){

    SDL_memset((str->buffer+str->front_size),'0',str->gap_size);

}

