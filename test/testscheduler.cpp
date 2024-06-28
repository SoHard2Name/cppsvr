#include "cppsvr/cppsvr.h"

// 这个主要测试了各种基本情况下 scheduler 能够正常运行，所
// 以包括那些 tickle(唤醒)、idle(空闲) 都是比较特殊的实现。

void test_fiber() {
	INFO("test in fiber");
	sleep(1);
	static int iCount = 5;
	if (--iCount) {
		cppsvr::Scheduler::GetThis()->schedule(&test_fiber);
	}
}

int main() {
	INFO("main");
	cppsvr::Scheduler sc(3, true, "main");
	sc.start();
	sleep(1);
	INFO("schedule");
	sc.schedule(&test_fiber);
	sc.stop();
	INFO("over");
	return 0;
}

/*

2024-06-28 11:02:05 820 (57003,UNKNOW; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:16 main
2024-06-28 11:02:05 821 (57004,main_0; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:129 run
2024-06-28 11:02:05 821 (57004,main_0; 2) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:232 idle
2024-06-28 11:02:05 822 (57005,main_1; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:129 run
2024-06-28 11:02:05 822 (57005,main_1; 3) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:232 idle
2024-06-28 11:02:06 821 (57003,main; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:20 schedule
2024-06-28 11:02:06 821 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:06 822 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:06 822 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:06 822 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:06 822 (57003,main; 1) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:122 Call one fiber. id 1
2024-06-28 11:02:06 822 (57003,main; 1) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:129 run
2024-06-28 11:02:06 822 (57003,main; 5) [INFO] XXX/cppsvr/test/testscheduler.cpp:7 test in fiber
2024-06-28 11:02:07 822 (57003,main; 5) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:07 822 (57003,main; 4) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:232 idle
2024-06-28 11:02:07 822 (57005,main_1; 6) [INFO] XXX/cppsvr/test/testscheduler.cpp:7 test in fiber
2024-06-28 11:02:08 823 (57005,main_1; 6) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:08 823 (57004,main_0; 7) [INFO] XXX/cppsvr/test/testscheduler.cpp:7 test in fiber
2024-06-28 11:02:09 825 (57005,main_1; 6) [INFO] XXX/cppsvr/test/testscheduler.cpp:7 test in fiber
2024-06-28 11:02:09 825 (57004,main_0; 7) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:10 827 (57005,main_1; 6) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:223 tickle
2024-06-28 11:02:10 827 (57003,main; 5) [INFO] XXX/cppsvr/test/testscheduler.cpp:7 test in fiber
2024-06-28 11:02:11 828 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:208 idle fiber term
2024-06-28 11:02:11 828 (57003,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:105 main work fiber call end. state 3
2024-06-28 11:02:11 828 (57004,main_0; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:208 idle fiber term
2024-06-28 11:02:11 829 (57005,main_1; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:208 idle fiber term
2024-06-28 11:02:11 829 (57003,main; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:23 over

*/