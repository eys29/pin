#include <cstdio>
#include <random>
#include <ctime>    
#include <cstdlib>  
#include <cfloat>

int max;
int min;

void ixy(){
    FILE *ixy;
    ixy = fopen("ixy.txt", "w");

    srand(time(0));
	const int N = 4096;
    // X
	for (int i = 0; i < N; ++i)
	{
		fprintf(ixy, "%d ", rand() % (max - min) + min);
	}
    fprintf(ixy, "\n");
    // Y
    for (int i = 0; i < N; ++i)
	{
		fprintf(ixy, "%d ", rand() % (max - min) + min);
	}
}

void fxy(){
    FILE *fxy;
    fxy = fopen("fxy.txt", "w");

    const int N = 4096;
	std::random_device rd; std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(min, max);
    // X
	for (int i = 0; i < N; ++i)
	{
		fprintf(fxy, "%.10f ", dis(gen));
	}
    fprintf(fxy, "\n");
    // Y
    for (int i = 0; i < N; ++i)
	{
		fprintf(fxy, "%.10f ", dis(gen));
	}
}

void dxy(){
    FILE *dxy;
    dxy = fopen("dxy.txt", "w");

    const int N = 4096;
	std::random_device rd; std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(min, max);
    // X
	for (int i = 0; i < N; ++i)
	{
		fprintf(dxy, "%.20lf ", dis(gen));
	}
    fprintf(dxy, "\n");
    // Y
    for (int i = 0; i < N; ++i)
	{
		fprintf(dxy, "%.20lf ", dis(gen));
	}
}

int main(int argc, char* argv[]){
	min = std::stoi(argv[1]);
	max = std::stoi(argv[2]);
    ixy();
    fxy();
    dxy();
}