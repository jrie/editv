#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_iostream.h"
#include "storage.h"
#include <stdio.h>


void storage_free(Storage* str){
    SDL_free(str->buffer);
    SDL_free(str);
}

Storage* storage_alloc(size_t size){

    Storage *s = SDL_malloc(sizeof(Storage));

    if (size == 0) {
        
        
        printf("Cannot create buffer with size of 0\n");
        return NULL;
    }

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
    s->buffer[0] = '\n';
    return s;

}

Storage* storage_alloccopy(const char* string, size_t size) {

    Storage* s = storage_alloc(size);
    
    SDL_memcpy(s->buffer, string, size);


    return s;

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

int storage_insert_c(Storage * str, char c, size_t pos){

    
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

    

    return 1;
}

int storage_insert(Storage * str, size_t pos,const char *s, size_t length){

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

    return 1;
}


int storage_remove(Storage * str, size_t pos, size_t count){

    if(pos+count > STR_END(str)){
        return 0;
    }

    if(str->front_size != pos+count){
        storage_move_gap(str,pos+count);
    }

    //dont actually have to do any allocating, just mark the area as SDL_free
    str->front_size-=count;
    str->gap_size+=count;

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



void storage_replaceall(Storage *str, char *target, char* s){

    int names[512];

    int c = storage_match(str,names,sizeof(names) / sizeof(names[0]),target);

    size_t targetlength = SDL_strlen(target);

    size_t slength = SDL_strlen(s);

    for (int i = c; i > 0; i--)
    {
        storage_remove(str,names[i-1],targetlength);
        storage_insert(str,names[i-1],s,slength);
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

