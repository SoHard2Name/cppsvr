#include "iostream"
#include "cppsvr/cppsvr.h"

// 测试同步 logger

int main() {
	DEBUG("begin main()");
	for (int i = 0; i < 5; i++) {
		INFO("succ %d", i);
	}
	std::string sErrMsg = "damn";
	ERROR("%s, man", sErrMsg.c_str());

	return 0;
}