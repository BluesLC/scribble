#include <QCoreApplication>
#include <iostream>
using namespace std;


class Father{
public:
    Father()=default;
    ~Father()=default;
    void publicFunc(){
        cout<<"Father publicFunction";
    }
protected:
    void protectedFunc(){
        cout<<"Father protectedFunction";
    }
private:
    void privateFunc(){
        cout<<"Father privateFunction";
    }

};


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Father* fa=new Father();

    fa->publicFunc();
    //fa->protectedFunc();无法访问
    //fa->privateFunc();无法访问
    return a.exec();
}
