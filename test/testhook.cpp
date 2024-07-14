#include "bits/stdc++.h"
using namespace std;

time_t time (time_t *__timer) {
	return 123456;
}

int main(){
	cout << time(nullptr) << endl;
	
}
/*

静态真的可以这么就直接把系统调用 hook 掉了

abcpony@ubuntu:~/QQMail/cppsvr/test$ g++ -o testhook testhook.cpp 
abcpony@ubuntu:~/QQMail/cppsvr/test$ ./testhook 
123456


动态 hook 则也是类似编写，不过需要通过设置让它在运行时搜索、链接的时候优先搜这个而不搜系统调用。

*/