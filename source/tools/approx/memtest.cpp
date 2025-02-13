#include <iostream>

/**
 * compile:
 * g++ memtest.cpp -o memtest
 * g++ memtest.cpp -S
 */

#define MAGIC_INSTR __asm__("xchg %eax,%eax;");

int main()
{
    MAGIC_INSTR;
    int image[100000];
    /** 
    // 600,002 mem instructions in this for loop
    // 1 iter: 5 reads + 3 writes = 8
    // 2 iter: 9 reads + 5 writes = 14
    // 3 iter: 13 reads + 7 writes = 20
    // 4 iter: 17 reads + 9 writes = 26


    // each iter: 4 reads + 2 writes = 6
    //      for loop: 3 reads + 1 writes = 4
    //                  read i, write i++, 
    //                  2 other reads? read in i++, read i < 100
    //      loop content: 1 read + 1 write = 2
    //                     read i, write image[i]

    // i is stack not a register
    // try with -o 
    // also look at assembly

    // initial: 1 read + 1 write = 2
    //                     write i
    //          1 read? i < 100
    */
    for (int i = 5; i < 100; i++) 
    {
        image[i] = (int)i; // 1 read + 1 write = 2
    }
    
    int sum = 0; // 1 write 
    int i = 0;
    // sum += image[i]; // 3 reads + 1 write 
    /** 
    // 700,002 mem instructions in this for loop 

    // 1 iter: 6 reads + 4 writes = 10
    // 2 iter: 11 reads + 6 writes = 17
    // 3 iter: 16 reads + 8 writes = 24
    // 4 iter: 21 reads + 10 writes = 31 
    // each iter: 5 reads + 2 writes = 7
    */
    for (int i = 0; i < 100; i++)
    {
        sum += image[i]; // 4 instrs
    }
    MAGIC_INSTR;
    printf("%d\n", sum);
    return 0;
}