#include <iostream>
using namespace std;
extern "C" {
	#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avcodec.lib")

int main(int argc, char* argv[]) {
	cout << "test" << endl;
	cout << avcodec_configuration()<< endl;
	getchar();
	return 0;
}
