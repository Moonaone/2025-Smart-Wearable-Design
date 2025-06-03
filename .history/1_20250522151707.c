#include "stdio.h"

int main()
{
    char str[5][100] = {"hello", "world", "i", "am", "a"};
    
    for(int i = 0; i < 5; i++)
    {
        printf("%s\n", str[i]); 
    }

    return 0;
}