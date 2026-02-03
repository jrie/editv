#pragma once
#include <stddef.h>

#define BUFFER_GAP_SIZE 512

#define STR_END(str) str->buffer_size - str->gap_size



typedef struct Storage
{
    char *buffer;

    size_t buffer_size;

    size_t front_size; // points to the index of the last char in front array

    size_t gap_size;

} Storage;



void storage_free(Storage* str);

Storage* storage_alloc(size_t size);
Storage* storage_alloccopy(const char* string, size_t size);

//index storage ignoring the gap
char storage_get(Storage* str, size_t index);


char* storage_ptr(Storage* str, size_t index);

void storage_move_gap(Storage* str, size_t pos);

int storage_grow(Storage* str);

int storage_realloc(Storage* str);

int storage_insert_c(Storage* str, char c, size_t pos);
int storage_insert(Storage* str, size_t pos, const char* s, size_t length);


int storage_remove(Storage* str, size_t pos, size_t count);


int storage_overwrite(Storage* str, size_t pos, char* s, size_t length);


//fills buffer with index of matching strings
//returns num found
int storage_match(Storage* str, int* buffer, size_t max, char* expr);


void storage_replaceall(Storage* str, char* target, char* s);



void print_buffer(Storage* str);
void print_buffer_separate(Storage* str);
void print_lines(Storage* str);

void storage_wipe_gap(Storage* str);