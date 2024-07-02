#include "cppsvr/threadpool.h"
#include "cppsvr/cppsvrconfig.h"
#include "cppsvr/commfunctions.h"

namespace cppsvr {

ThreadPool::ThreadPool() : m_bIsStopping(false), m_iThreadNum(CppSvrConfig::GetSingleton()->GetThreadPoolThreadNum()) {
	for (int i = 0; i < m_iThreadNum; i++) {
		m_vecThreads.emplace_back(new Thread(std::bind(&ThreadPool::Run, this), StrFormat("Thread_%d", i)));
	}
}

ThreadPool::~ThreadPool() {
	assert(ShouldStop());
}

void ThreadPool::AddTask(std::function<void()> funTask) {
	AddTask(std::make_shared<Coroutine>(std::move(funTask)));
}

void ThreadPool::AddTask(Coroutine::ptr pCoroutine) {
	Mutex::ScopedLock oLock(m_oTasksMutex);
	m_listTasks.push_back(pCoroutine);
	m_semNotEmpty.Notify();
	DEBUG("add one task succ. its cur use count %ld", m_listTasks.back().use_count());
}

void ThreadPool::Close() {
	m_bIsStopping = true;
	INFO("closing the thread poll");
	for (int i = 0; i < m_iThreadNum; i++) {
		m_semNotEmpty.Notify();
	}
	for (auto pThread : m_vecThreads) {
		pThread->Join();
	}
}

void ThreadPool::Run() {
	INFO("one worker thread begin Run");
	while (true) {
		// 从队列中取出一个任务。
		m_semNotEmpty.Wait(); // 首先要有任务
		if (ShouldStop()) { // 是叫我停下来的。注意在这个步骤之前不能先拿锁。
			break;
		}
		Coroutine::ptr oTask;
		{
			Mutex::ScopedLock oLock(m_oTasksMutex);
			if (m_listTasks.size()) {
				oTask = m_listTasks.front();
				m_listTasks.pop_front();
			}
		}
		DEBUG("task use count before exec: %ld", oTask.use_count());
		if (oTask) {
			oTask->SwapIn();
		}
		// 到这里其实也已经除出了那个协程，走到了主要工作协程的地方了。
		DEBUG("task coroutine use count %ld", oTask.use_count());
	}
	ERROR("one thread end Run");
}

bool ThreadPool::ShouldStop() {
	Mutex::ScopedLock oLock(m_oTasksMutex);
	return m_bIsStopping && m_listTasks.empty();
}

}
