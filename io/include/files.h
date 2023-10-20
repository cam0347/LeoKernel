#pragma once
#include <include/types.h>
#define MAX_FILES 2048
#define STDBUFF_SIZE 2048
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

typedef enum {
    data,
    dev,
    stream
} file_type_t;

typedef struct {
    /*
    incomplete struct
    */
   file_type_t type;
   uint32_t size;
   void *ptr;
} file_t;

extern file_t stdin;
extern file_t stdout;
extern file_t stderr;

bool init_files();
void init_file_descriptor(file_t *file, file_type_t type);
file_t *get_file(uint32_t fd);
uint64_t write(uint32_t fd, void *buffer, uint64_t count);
uint64_t read(uint32_t fd, void *buffer, uint64_t count);