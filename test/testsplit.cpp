#include "cppsvr/cppsvr.h"


int main() {
	
	// 这套已经过时了。是自己异想天开的东西。

	// cppsvr::Thread::SetThreadName("Main");
	// INFO("test begin");
	// cppsvr::ThreadPool::GetSingleton()->AddTask([]() {
	// 	INFO("before split");
	// 	DEBUG("before split use count %ld", cppsvr::Coroutine::GetThis().use_count());
	// 	cppsvr::Coroutine::GetThis()->Split();
	// 	INFO("after split");
	// });
	// cppsvr::ThreadPool::GetSingleton()->Close();
	// INFO("end test, current coroutine num: %lu", cppsvr::Coroutine::TotalCoroutines());
	
	return 0;
}

/*

任务可以分两步走，且协程的销毁之类的事情都正常。
可以注意一下最后那几个销毁，其实内核线程是已经在 close 的时候被销毁了，那里销毁的是维护线程信息的 Thread 对象。

2024-06-30 09:49:41 158 (120884,Main; 0) [INFO] XXX/cppsvr/test/testsplit.cpp:6 test begin
2024-06-30 09:49:41 158 (120884,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:26 thread Thread_0 be constructed succ.
2024-06-30 09:49:41 158 (120885,Thread_0; 0) [INFO] XXX/cppsvr/cppsvr/threadpool.cpp:40 one worker thread begin Run
2024-06-30 09:49:41 158 (120884,Thread_1; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:26 thread Thread_1 be constructed succ.
2024-06-30 09:49:41 158 (120886,Thread_1; 0) [INFO] XXX/cppsvr/cppsvr/threadpool.cpp:40 one worker thread begin Run
2024-06-30 09:49:41 159 (120884,Thread_2; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:26 thread Thread_2 be constructed succ.
2024-06-30 09:49:41 159 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:26 thread Thread_3 be constructed succ.
2024-06-30 09:49:41 159 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:69 one son coroutine construct succ
2024-06-30 09:49:41 159 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:25 add one task succ. its cur use count 2
2024-06-30 09:49:41 159 (120884,Thread_3; 0) [INFO] XXX/cppsvr/cppsvr/threadpool.cpp:30 closing the thread poll
2024-06-30 09:49:41 159 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:55 task use count before exec: 1
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:97 before main func, use count 2
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:162 begin main func, use count 2
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [INFO] XXX/cppsvr/test/testsplit.cpp:8 before split
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/test/testsplit.cpp:9 before split use count 3
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:25 add one task succ. its cur use count 5
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:106 cur use count 5
2024-06-30 09:49:41 159 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:60 task coroutine use count 4
2024-06-30 09:49:41 159 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:55 task use count before exec: 3
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:97 before main func, use count 4
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [INFO] XXX/cppsvr/test/testsplit.cpp:11 after split
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:171 here555 use count 2
2024-06-30 09:49:41 159 (120885,Thread_0; 1) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:106 cur use count 2
2024-06-30 09:49:41 159 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/threadpool.cpp:60 task coroutine use count 1
2024-06-30 09:49:41 160 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:79 one son coroutine destruct succ.
2024-06-30 09:49:41 160 (120885,Thread_0; 0) [ERROR] XXX/cppsvr/cppsvr/threadpool.cpp:62 one thread end Run
2024-06-30 09:49:41 160 (120885,Thread_0; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-06-30 09:49:41 160 (120886,Thread_1; 0) [ERROR] XXX/cppsvr/cppsvr/threadpool.cpp:62 one thread end Run
2024-06-30 09:49:41 160 (120886,Thread_1; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-06-30 09:49:41 160 (120888,Thread_3; 0) [INFO] XXX/cppsvr/cppsvr/threadpool.cpp:40 one worker thread begin Run
2024-06-30 09:49:41 161 (120888,Thread_3; 0) [ERROR] XXX/cppsvr/cppsvr/threadpool.cpp:62 one thread end Run
2024-06-30 09:49:41 161 (120888,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-06-30 09:49:41 159 (120887,Thread_2; 0) [INFO] XXX/cppsvr/cppsvr/threadpool.cpp:40 one worker thread begin Run
2024-06-30 09:49:41 161 (120887,Thread_2; 0) [ERROR] XXX/cppsvr/cppsvr/threadpool.cpp:62 one thread end Run
2024-06-30 09:49:41 161 (120887,Thread_2; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-06-30 09:49:41 161 (120884,Thread_3; 0) [INFO] XXX/cppsvr/test/testsplit.cpp:14 end test, current coroutine num: 5
2024-06-30 09:49:41 161 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-06-30 09:49:41 162 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:36 thread Thread_0 be destroyed.
2024-06-30 09:49:41 162 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:36 thread Thread_1 be destroyed.
2024-06-30 09:49:41 162 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:36 thread Thread_2 be destroyed.
2024-06-30 09:49:41 162 (120884,Thread_3; 0) [DEBUG] XXX/cppsvr/cppsvr/thread.cpp:36 thread Thread_3 be destroyed.

*/