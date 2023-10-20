#pragma once
#include <include/types.h>

#define STR_EQUAL 0
#define STR_DIFF  1

void strcat(char *, const char *);
void strcpy(char *, const char *);
void *strncpy(char *dest, char *src, uint32_t n);
uint32_t strlen(const char *);
uint32_t strcmp(const char *, const char *);
int strncmp(char *str1, char *str2, uint32_t n);
char *strtok(const char *, const char *);
int stoi(char *);