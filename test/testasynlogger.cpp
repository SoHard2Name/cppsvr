#include "cppsvr/cppsvr.h"
#include "vector"

// 这个主要是测试下加锁后的日志器是否能正常工作于多线程环境。

void TestThread() {
	INFO("one test thread begin");
	cppsvr::SleepMs(10);
	INFO("one test thread end");
}

int main() {
	cppsvr::Thread::SetThreadName("main");
	cppsvr::CppSvrConfig::GetSingleton()->LogConfigInfo();
	const int iWorkerThreadNum = 20;
	std::vector<cppsvr::Thread::ptr> vecThreads(iWorkerThreadNum);
	for (int i = 0; i < iWorkerThreadNum; i++) {
		vecThreads[i] = std::make_shared<cppsvr::Thread>(TestThread, cppsvr::StrFormat("TestThread_%d", i));
	}
	for (auto &oThread : vecThreads) {
		oThread->Join();
	}
	INFO("test succ.");
	return 0;
}