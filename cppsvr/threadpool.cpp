#include "cppsvr/threadpool.h"
#include "cppsvr/cppsvrconfig.h"
#include "cppsvr/commfunctions.h"

namespace cppsvr {

ThreadPool::ThreadPool() : m_bIsStopping(false), m_iThreadNum(CppSvrConfig::GetSingleton()->GetThreadNum()),
						   m_iCoroutineNum(CppSvrConfig::GetSingleton()->GetCoroutineNum()) {
	for (int i = 0; i < m_iThreadNum; i++) {
		m_vecThreads.emplace_back(new Thread(std::bind(&ThreadPool::Run, this), StrFormat("Thread_%d", i)));
	}
}

ThreadPool::~ThreadPool() {
	assert(ShouldStop());
}

void ThreadPool::AddTask(std::function<void()> funTask) {
	Mutex::ScopedLock oLock(m_oTasksMutex);
	m_listTasks.push_back(std::move(funTask));
	m_semNotEmpty.Notify();
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
	// 创建本线程的协程池
	for (int i = 0; i < m_iCoroutineNum; i++) {
		Thread::GetThis()->m_vecIdleCoroutine.emplace_back();
	}
	while (true) {
		// 从队列中取出一个任务。
		m_semNotEmpty.Wait(); // 首先要有任务
		if (ShouldStop()) { // 是叫我停下来的。注意在这个步骤之前不能先拿锁。
			break;
		}
		std::function<void()> oTask;
		{ // TODO: 改为无锁队列
			Mutex::ScopedLock oLock(m_oTasksMutex);
			if (m_listTasks.size()) {
				oTask = std::move(m_listTasks.front());
				m_listTasks.pop_front();
			}
		}
		if (oTask) {
			oTask();
		}
	}
	ERROR("one thread end Run");
}

bool ThreadPool::ShouldStop() {
	Mutex::ScopedLock oLock(m_oTasksMutex);
	return m_bIsStopping && m_listTasks.empty();
}

}
