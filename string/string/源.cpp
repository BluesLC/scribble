#include<string>
#include<iostream>
using namespace std;

int main() {
	string a = "123456";
	int b =0;
	for (auto &r : a) {
		b += (r - '0');
	}
	cout << b;
	system("pause");
	return 0;
}

