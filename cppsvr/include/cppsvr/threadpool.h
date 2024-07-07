#pragma once
#include "singleton.h"
#include "vector"
#include "list"
#include "thread.h"
#include "coroutine.h"
#include "mutex.h"
#include "functional"
#include "coroutine.h"

// 注意，这里虽然叫做线程池，但其实是更准确来说是协程池。
// 每个线程里面会有一个协程池，这个是主要的任务执行单位。

// 想来想去还是要把每个线程都做成响应器比较合理，比如一个工作线程，
// 它就拥有一个协程池，然后维护一些连接，这些连接是主响应器 accept
// 获得的，获得之后就会注册给分配给维护连接数最少的线程。
// 因为 epoll 是本线程的，你在处理回调函数的时候也更好弄，比如一个
// SwapIn 的需求，直接就可以做到了，而如果是注册到了别的线程的回调
// 函数，实际上还是要提醒一下这个线程，只有它才能去执行那个协程，否
// 则需要维护全局协程池，这样则又多了一些竞争。

namespace cppsvr {

// 一个进程只维护一个线程池是很合理的事情。
class ThreadPool {
	SINGLETON(ThreadPool);
public:
	void AddTask(std::function<void()> funTask);
	void AddTask(Coroutine::ptr pCoroutine);
	void Close();
	
private:
	void Run();
	bool ShouldStop();
	
private:
	bool m_bIsStopping;
	uint32_t m_iThreadNum;
	std::vector<Thread::ptr> m_vecThreads;
	Semaphore m_semNotEmpty;
	Mutex m_oTasksMutex;
	std::list<std::function<void()>> m_listTasks;
};

}