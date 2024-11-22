#include <iostream>

#define MAGIC_EAX_INSTRUCTION __asm__ __volatile__("xchg %eax,%eax;");
#define MAGIC_ECX_INSTRUCTION __asm__ __volatile__("xchg %ecx,%ecx;");

int main()
{
    MAGIC_ECX_INSTRUCTION;
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
    MAGIC_ECX_INSTRUCTION;

    return 0;
}