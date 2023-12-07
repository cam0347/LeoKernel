#include <include/string.h>
#include <include/types.h>
#include <include/mem.h>
#include <tty/include/tty.h>
#include <include/math.h>
#include <io/include/files.h>

//catenate src to dest, returns the pointer to destination
void strcat(char *dest, const char *src) {
    dest += strlen(dest); //move the pointer at the end of the string

    for (uint32_t i = 0; i < strlen(src); i++) {
        *(dest + i) = *(src + i);
    }
}

//copy src string into dest
void strcpy(char *dest, const char *src) {
    uint32_t i = 0;

    while (*(src + i) != 0x0) {
        *(dest + i) = *(src + i);
        i++;
    }
}

void *strncpy(char *dest, char *src, uint32_t n) {
    uint32_t i = 0;
    uint32_t src_len = strlen(src);

    while(i < n) {
        *(dest + i) = i < src_len ? *(src + i) : '\0';
        i++;
    }

    return (void *) dest;
}

//returns the length of the string str (terminator is not included in the output)
uint32_t strlen(const char *str) {
    uint32_t i = 0;

    while(*(str + i) != 0x00) {
        i++;
    }

    return i;
}

uint32_t strcmp(const char *str1, const char *str2) {
    uint32_t str1_l = strlen(str1);
    uint32_t str2_l = strlen(str2);

    if (str1_l != str2_l) {
        return -1;
    }

    for (uint32_t i = 0; i < str1_l; i++) {
        if (*(str1 + i) != *(str2 + i)) {
            return -1;
        }
    }

    return 0;
}

int strncmp(char *str1, char *str2, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (*(str1 + i) > *(str2 + i)) {
            return 1;
        } else if (*(str1 + i) < *(str2 + i)) {
            return -1;
        }
    }

    return 0;
}

//returns the first substring starting with the token or null if there's an error
char *strtok(const char *str, const char *token) {
    uint32_t tok_l = strlen(token);
    uint32_t str_l = strlen(str);
    char buffer[tok_l + 1];
    buffer[tok_l] = 0x00;

    if (tok_l == 0 || str_l == 0) {
        return null;
    }

    if (str_l < tok_l) {
        return null;
    } else if (str_l == tok_l) {
        return (char *) str;
    }

    for (uint32_t i = 0; i < str_l - tok_l; i++) {
        memcpy(buffer, (void *)(str + i), tok_l);

        if (strcmp(buffer, token) == STR_EQUAL) {
            return (char *)(str + i);
        }
    }

    return null;
}

//convert string to signed integer
int stoi(char *string) {
    int ret = 0;
    uint32_t length = strlen(string);
    bool neg = false;

    if (*string == '-') {
        neg = true;
        memmove(string, string + 1, length);
        length--;
    }

    for (uint32_t i = 0; i < length; i++) {
        if (*(string + i) >= 48 && *(string + i) <= 57) {
            ret += (*(string + length - 1 - i) - 48) * pow(10, i);
        } else {
            return 0;
        }
    }

    if (neg) {
        ret *= -1;
    }

    return ret;
}