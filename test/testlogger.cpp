#include "iostream"
#include "utils/logger.h"
#include "utils/commfunctions.h"

// 测试同步 logger

int main() {
	DEBUG("begin main()");
	for (int i = 0; i < 5; i++) {
		INFO("succ %d", i);
		std::cout << utility::StrFormat("succ %d", i) << " ...... \n";
	}
	std::string sErrMsg = "damn";
	ERROR("%s, man", sErrMsg.c_str());

	return 0;
}