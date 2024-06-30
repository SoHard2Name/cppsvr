#pragma once
#include "singleton.h"
#include "vector"
#include "list"
#include "thread.h"
#include "coroutine.h"
#include "mutex.h"
#include "functional"
#include "coroutine.h"

namespace cppsvr {

// 一个进程只维护一个线程池是很合理的事情。
class ThreadPool {
	SINGLETON(ThreadPool);
public:
	void AddTask(std::function<void()> funTask);
	void AddTask(cppsvr::Coroutine::ptr pCoroutine);
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
	std::list<Coroutine::ptr> m_listTasks;
};

}