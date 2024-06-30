// #include "iostream"
// #include "cppsvr/cppsvr.h"

// // 注意到目前还是同步日志，所以文件内容是乱的，但是控制台输出还是正常的。
// // 正常可能是因为 std::cout << xxx << std::endl; 底层有限制加锁？

// // fiber 的逻辑就是每个线程里面先创建一个主协程，用于其他子协程的出入（
// // 它们开始执行或挂起或结束就是和主协程交换上下文）。其实本身也是有一个
// // 主线的，只不过要创建这么个具体的变量来理解、切换更方便。

// void RunInCoroutine() {
// 	INFO("RunInCoroutine begin");
// 	cppsvr::Coroutine::YieldToHold();
// 	INFO("RunInCoroutine end");
// 	cppsvr::Coroutine::YieldToHold();
// }

// void TestCoroutine() {
// 	INFO("main begin -1");
// 	{
// 		// 这个是创建主协程用的，不然就会寄掉。
// 		cppsvr::Coroutine::GetThis();
// 		INFO("main begin");
// 		cppsvr::Coroutine::ptr oCoroutine(new cppsvr::Coroutine(RunInCoroutine));
// 		oCoroutine->SwapIn(); // 这里开始调用这个协程（但是运行一半被上面第一个 YieldToHold 给挂起了）
// 		INFO("main after SwapIn"); // 回到主协程
// 		oCoroutine->SwapIn(); // 继续调用，不过又被第二个 YieldToHold 挂起
// 		INFO("main after end"); // 回到主协程
// 		oCoroutine->SwapIn(); // 继续调用，这次就顺利走完回调函数了。
// 	} // 到这里，括号内创建的子协程 fiber 会被销毁
// 	INFO("main after end2");
// } // 到这里销毁主协程。到 main 函数结束后销毁线程。

// int main(int argc, char **argv) {
// 	cppsvr::CppSvrConfig::GetSingleton()->LogConfigInfo();
	
// 	// 主线程。
// 	cppsvr::Thread::SetThreadName("main");
	
// 	std::vector<cppsvr::Thread::ptr> vecThreads;
// 	for (int i = 0; i < 3; ++i) {
// 		vecThreads.push_back(cppsvr::Thread::ptr(
// 			new cppsvr::Thread(&TestCoroutine, "name_" + std::to_string(i))));
// 	}
// 	for (auto oThread : vecThreads) {
// 		oThread->Join();
// 	}
// 	return 0;
// }
// /*

// 下面搜 name_0 就能理解了。

// 2024-06-26 10:54:17 (121743,UNKNOW) [INFO] XXX/cppsvr/cppsvr/cppsvrconfig.cpp:23 config info: log file name ./logs/cppsvr.log, log max size 8192, log console 1, log level INFO[2]. fiber stack size 2097152
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:23 thread name_0 be constructed succ.
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:23 thread name_1 be constructed succ.
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:16 main begin -1
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:20 main begin
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:9 RunInCoroutine begin
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:23 main after swapIn
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:11 RunInCoroutine end
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:25 main after end
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:85 one son fiber destruct succ.
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/test/testfiber.cpp:28 main after end2
// 2024-06-26 10:54:17 (121744,name_0) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:94 one main fiber destruct succ.
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:23 thread name_2 be constructed succ.
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:16 main begin -1
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:20 main begin
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:9 RunInCoroutine begin
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:23 main after swapIn
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:11 RunInCoroutine end
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:25 main after end
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:85 one son fiber destruct succ.
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/test/testfiber.cpp:28 main after end2
// 2024-06-26 10:54:17 (121745,name_1) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:94 one main fiber destruct succ.
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:16 main begin -1
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:20 main begin
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:9 RunInCoroutine begin
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:23 main after swapIn
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:11 RunInCoroutine end
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:25 main after end
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:85 one son fiber destruct succ.
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/test/testfiber.cpp:28 main after end2
// 2024-06-26 10:54:17 (121746,name_2) [INFO] XXX/cppsvr/cppsvr/fiber.cpp:94 one main fiber destruct succ.
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:35 thread name_0 be destroyed.
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:35 thread name_1 be destroyed.
// 2024-06-26 10:54:17 (121743,main) [INFO] XXX/cppsvr/cppsvr/thread.cpp:35 thread name_2 be destroyed.

// */