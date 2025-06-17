#include <cstdio>
#include <random>
#include <ctime>    
#include <cstdlib>  
#include <sstream>
#include <iostream>
#include <fstream>


using namespace std;

int main()
{

	const int N = 4096;
	int X[N], Y[N], alpha = 2;
    
    ifstream file("ixy.txt");
    std::string str;
	// read X 
    getline(file, str);
	stringstream ss_x(str);
	string word_x;
	int i_x = 0;
	while (getline(ss_x, word_x, ' ')) {
		X[i_x] = stoi(word_x); 
		i_x++;
	}
	// read Y
	getline(file, str);
	stringstream ss_y(str);
	string word_y;
	int i_y = 0;
	while (getline(ss_y, word_y, ' ')) {
		Y[i_y] = stoi(word_y); 
		i_y++;
	}

	for (int i=0; i < 2; i++){
		int tempx = X[i];
		int tempy = Y[i];
		asm("xchg %ecx,%ecx;"); // magic instruction
		int xi = tempx;
		int yi = tempy;
		asm("xchg %ecx,%ecx;"); // magic instruction

		Y[i] = alpha * xi + yi;
	}

	int sum = 0;
	for (int i=0; i < 2; i++){
		sum += Y[1];

	}
	printf("%d\n", sum);
	return 0;
}