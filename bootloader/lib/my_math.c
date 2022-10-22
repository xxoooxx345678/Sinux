#include "my_math.h"

int pow(int x, int y)
{
    int res = 1;
    while (y > 0)
    {
        if (y & 1)
            res *= x;
        y >>= 1;
        x *= x;
    }
    return res;
}