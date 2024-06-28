#include "cppsvr/cppsvr.h"

// 这个主要测试了各种基本情况下 scheduler 能够正常运行，所
// 以包括那些 tickle(唤醒)、idle(空闲) 都是比较特殊的实现。

void test_fiber() {
	INFO("test in fiber");
}

int main() {
	INFO("main");
	cppsvr::Scheduler sc(1, true, "main");
	sc.schedule(&test_fiber);
	INFO("schedule");
	sc.start();
	sc.stop();
	INFO("over");
	return 0;
}

/*

2024-06-28 05:18:46 661 (51219,UNKNOW; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:8 main
2024-06-28 05:18:46 661 (51219,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:206 tickle
2024-06-28 05:18:46 662 (51219,main; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:11 schedule
2024-06-28 05:18:46 662 (51219,main; 1) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:122 Call one fiber. id 1
2024-06-28 05:18:46 662 (51219,main; 1) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:117 run
2024-06-28 05:18:46 662 (51219,main; 3) [INFO] XXX/cppsvr/test/testscheduler.cpp:4 test in fiber
2024-06-28 05:18:46 662 (51219,main; 2) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:215 idle
2024-06-28 05:18:46 662 (51219,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:191 idle fiber term
2024-06-28 05:18:46 662 (51219,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:72 main work fiber call end. state 3
2024-06-28 05:18:46 662 (51219,main; 0) [INFO] XXX/cppsvr/cppsvr/scheduler.cpp:79 scheduler stop
2024-06-28 05:18:46 662 (51219,main; 0) [INFO] XXX/cppsvr/test/testscheduler.cpp:14 over

*/