#include <cstdio>
#include <random>
#include <cstdlib>  
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

int main()
{
	const int N = 4096;
	float X[N], Y[N], alpha = 0.5;
	
    ifstream file("fxy.txt");
    std::string str;
	// read X 
    getline(file, str);
	stringstream ss_x(str);
	string word_x;
	int i_x = 0;
	while (getline(ss_x, word_x, ' ')) {
		X[i_x] = stof(word_x); 
		i_x++;
	}
	// read Y
	getline(file, str);
	stringstream ss_y(str);
	string word_y;
	int i_y = 0;
	while (getline(ss_y, word_y, ' ')) {
		Y[i_y] = stof(word_y); 
		i_y++;
	}

	// Start of daxpy loop
	for (int i = 0; i < N; ++i)
	{
		float tempx = X[i];
		float tempy = Y[i];
		asm("xchg %ecx,%ecx;"); // magic instruction
		float xi = tempx;
		float yi = tempy;
		asm("xchg %ecx,%ecx;"); // magic instruction

		Y[i] = alpha * xi + yi;
	}
	// End of daxpy loop

	float sum = 0;
	for (int i = 0; i < N; ++i)
	{
		sum += Y[i];
	}
	printf("%f\n", sum);
	return 0;
}