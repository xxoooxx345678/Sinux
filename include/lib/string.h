#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcat(char *dest, const char *src);
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
void *memcpy(void *dest, const void *src, size_t len);
int atoi(char *str);
void *memset(void *s, int c, size_t n);
char *strtok(char *s, char *delim);

#endif