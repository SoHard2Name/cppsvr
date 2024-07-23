#pragma once
#include "vector"
#include "coroutine.h"
#include "thread.h"
#include "cppsvrconfig.h"
#include "sys/epoll.h"
#include "timer.h"

namespace cppsvr {

// 协程池，每个线程里面只会有一个。
// 原来有 SO_REUSEPORT 这种东西，直接就允许多个（线程
// 的）fd 去监听同一个 ip-port，由内核来实现负载均衡。

#define MUST_WAIT_THREAD_IN_EVERY_SON_CLASS_DESTRCUTOR_FIRST_LINE    \
	if (m_pThread) {    \
		m_pThread->Join();    \
		delete m_pThread;    \
		m_pThread = nullptr;    \
	}

class CoroutinePool {
public:
	CoroutinePool(uint32_t iCoroutineNum = CppSvrConfig::GetSingleton()->GetCoroutineNum());
	virtual ~CoroutinePool();
	void Run();
	void AddActive(TimeEvent::ptr pTimeEvent);

	static CoroutinePool *GetThis();
	static void SetThis(CoroutinePool *pCurrentPool);
	// 事件默认的 prepare 和 process
	static void DefaultPrepare(TimeEvent::ptr pTimeEvent);
	static void DefaultProcess(Coroutine *pCoroutine);
	static void WaitFdEventWithTimeout(int iFd, int iEpollEvent, uint32_t iRelativeTimeout);

protected:
	// 子类自己设置各个协程要执行的函数
	virtual void InitCoroutines();

private:
	void ThreadRun();

protected:
	int m_iId;
	int m_iEpollFd;
	uint32_t m_iCoroutineNum;
	std::vector<Coroutine*> m_vecCoroutine;
	Thread* m_pThread;
	Timer m_oTimer;
	std::list<TimeEvent::ptr> m_listActiveEvent;
};


}
