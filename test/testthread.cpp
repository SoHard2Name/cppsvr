#include "cppsvr/cppsvr.h"

int main() {
	INFO("begin main()");
	cppsvr::Thread oThread([&](){
		INFO("thread 1 begin");
		INFO("Hello world!");
		cppsvr::SleepMs(500);
		INFO("Goodbye world...");
		INFO("thread 1 end.");
	}, "TestThread_1");
	oThread.Join(); // 哥们目前还是同步日志不敢乱打，同时也测下这个 join 函数
	{
		cppsvr::Thread oThread([&]() {
			INFO("thread 2 begin");
			sleep(1);
			INFO("thread 2 end.");
		}, "TestThread_2");
	}
	INFO("this line lower than the 10th but faster than the 17th.");
	cppsvr::SleepMs(1100);
	INFO("test succ.");
	return 0;
}
/*
老铁没毛病
MSG: get logger conf succ. file name ./logs/cppsvr.log, max size 8192, console 1, level DEBUG
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:4 begin main()
2024-06-23 08:32:44 [INFO] XXX/cppsvr/cppsvr/thread.cpp:23 thread TestThread_1 be constructed succ.
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:6 thread 1 begin
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:7 Hello world!
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:9 Goodbye world...
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:10 thread 1 end.
2024-06-23 08:32:44 [INFO] XXX/cppsvr/cppsvr/thread.cpp:23 thread TestThread_2 be constructed succ.
2024-06-23 08:32:44 [INFO] XXX/cppsvr/cppsvr/thread.cpp:35 thread TestThread_2 be destroyed.
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:20 this line lower than the 10th but faster than the 17th.
2024-06-23 08:32:44 [INFO] XXX/cppsvr/test/testthread.cpp:15 thread 2 begin
2024-06-23 08:32:45 [INFO] XXX/cppsvr/test/testthread.cpp:17 thread 2 end.
2024-06-23 08:32:46 [INFO] XXX/cppsvr/test/testthread.cpp:22 test succ.
2024-06-23 08:32:46 [INFO] XXX/cppsvr/cppsvr/thread.cpp:35 thread TestThread_1 be destroyed.
*/