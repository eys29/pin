#include <iostream>

#define MAGIC_INSTR __asm__("xchg %eax,%eax;");

int main()
{
    MAGIC_INSTR;
    int image[100];
    for (int i = 0; i < 100; i++)
    {
        image[i] = i;
    }
    int sum = 0;
    for (int i = 0; i < 100; i++)
    {
        sum += image[i];
    }
    MAGIC_INSTR;
    return 0;
}