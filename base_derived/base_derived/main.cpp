#include<iostream>

using namespace std;

class Base {
public:
	Base() {
		cout << "new base" << endl;
	}
	virtual ~Base() {
		cout << "delete base" << endl;

	}
	virtual void test() {
		cout << "cout base" << endl;
	}
};

class Derived :public Base {
public:
	Derived() {
		cout << "new derived" << endl;
	}
	~Derived() {
		cout << "delete derived" << endl;
	}
	void test() {
		cout << "cout derived" << endl;
	}
};

class douDerived :public Derived {
public:
	douDerived() {
		cout << "new douderived" << endl;
	}
	~douDerived() {
		cout << "delete douderived" << endl;
	}
	void test() {
		cout << "cout douderived" << endl;
	}
};

int main() {
	Base *pbase = new douDerived();
	pbase->test();
	delete pbase;
	system("pause");
	return 0;
}