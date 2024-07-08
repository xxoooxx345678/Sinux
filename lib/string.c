#include <string.h>

int strcmp(const char *s1, const char *s2)
{
    while (*s1 || *s2)
    {
        if (*s1 == *s2)
        {
            ++s1;
            ++s2;
        }
        else
            return *s1 - *s2;
    }
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while ((*s1 || *s2) && n--)
    {
        if (*s1 == *s2)
        {
            ++s1;
            ++s2;
        }
        else
            return *s1 - *s2;
    }
    return 0;
}

char *strcat(char *dest, const char *src)
{
    char *cur = dest + strlen(dest);
    while (*src)
        *cur++ = *src++;
    *cur = '\0';
    return dest;
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++)
        len++;
    return len;
}

char *strcpy(char *dest, const char *src)
{
    char *tmp = dest;
    while (*src)
        *tmp++ = *src++;
    *tmp = 0; 
    return dest;
}

void *memcpy(void *dest, const void *src, size_t len)
{
    char *dest_tmp = (char *)dest;
    const char *src_tmp = (const char *)src;
    while (len--)
        *dest_tmp++ = *src_tmp++;
    return dest;
}

int atoi(char *str)
{
    int result = 0;
    int sign = 1;

    // Skip leading whitespace
    while (*str == ' ')
        str++;

    // Handle optional sign
    if (*str == '-' || *str == '+')
    {
        if (*str == '-')
            sign = -1;
        str++;
    }

    // Convert digits to integer
    while (*str >= '0' && *str <= '9')
    {
        int digit = *str - '0';

        result = result * 10 + digit;
        str++;
    }

    return sign * result;
}

void *memset(void *s, int c, size_t n)
{
    char *tmp = (char *)s;
    while (n--)
        *tmp++ = c;
    return s;
}