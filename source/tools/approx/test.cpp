#include <iostream>

int main() {
    int image[100];
    for (int i=0; i < 100; i++){
        image[i] = i;
    }
    int sum = 0;
    for (int i=0; i < 100; i++){
        sum += image[i];
    }
    return 0;
}