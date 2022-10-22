#ifndef __MY_STRING_H__
#define __MY_STRING_H__

#define MAX_LEN 256

char *itoa(int value, char *s);
int atoi(char *s);
unsigned int vsprintf(char *dst, char *fmt, __builtin_va_list args);
unsigned int sprintf(char *dst, char *fmt, ...);
int strcmp(const char *X, const char *Y);
char *strcat(char *dest, const char *src);
unsigned int strlen(const char * src);
void reverse(char *x, char *y);
void unsign_itohexa(unsigned long long x, char *res);

#endif