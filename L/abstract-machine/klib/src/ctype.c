#include <klib.h>

int isalpha(const int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

int isdigit(const int c) {
    return (c >= '0' && c <= '9');
}