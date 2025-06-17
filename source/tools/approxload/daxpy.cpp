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
	double X[N], Y[N], alpha = 0.5;
	
    ifstream file("dxy.txt");
    std::string str;
	// read X 
    getline(file, str);
	stringstream ss_x(str);
	string word_x;
	int i_x = 0;
	while (getline(ss_x, word_x, ' ')) {
		X[i_x] = stod(word_x); 
		i_x++;
	}
	// read Y
	getline(file, str);
	stringstream ss_y(str);
	string word_y;
	int i_y = 0;
	while (getline(ss_y, word_y, ' ')) {
		Y[i_y] = stod(word_y); 
		i_y++;
	}

	// Start of daxpy loop
	for (int i = 0; i < N; ++i)
	{
		double tempx = X[i];
		double tempy = Y[i];
		asm("xchg %ecx,%ecx;"); // magic instruction
		double xi = tempx;
		double yi = tempy;
		asm("xchg %ecx,%ecx;"); // magic instruction

		Y[i] = alpha * xi + yi;
	}
	// End of daxpy loop

	double sum = 0;
	for (int i = 0; i < N; ++i)
	{
		sum += Y[i];
	}
	printf("%lf\n", sum);
	return 0;
}