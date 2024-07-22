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
	void InitCoroutines();

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


#define RUN_FUNC_DECL(ClassName)    \
public:    \
	void Run();    \
private:    \
	void ThreadRun()

#define RUN_FUNC_IMPL(ClassName)     \
void ClassName::Run() {    \
	m_pThread = new cppsvr::Thread(std::bind(&ClassName::ThreadRun, this), cppsvr::StrFormat("CoroutunePool_%u_Thread", m_iId));    \
}    \
void ClassName::ThreadRun() {    \
	assert(!GetThis());    \
	SetThis(this);    \
	assert(!cppsvr::Timer::GetThis());    \
	cppsvr::Timer::SetThis(&m_oTimer);    \
	InitCoroutines();    \
	INFO("init coroutines end. begin event loop...");    \
	const int iEventsSize = 1024, iWaitTimeMs = 100;    \
	epoll_event *pEpollEvents = new epoll_event[iEventsSize];    \
	memset(pEpollEvents, 0, sizeof(epoll_event) * iEventsSize);    \
	while (true) {    \
		int iRet = epoll_wait(m_iEpollFd, pEpollEvents, iEventsSize, iWaitTimeMs);    \
		if (iRet < 0) {    \
			if (errno != EINTR) {    \
				ERROR("epoll_wait error. ret %d", iRet);    \
				break;    \
			}    \
			continue;    \
		}    \
		const int iEventNum = iRet;    \
		for (int i = 0; i < iEventNum; i++) {    \
			auto &oEpollEvent = pEpollEvents[i];    \
			auto pTimeEvent = *(cppsvr::TimeEvent::ptr*)oEpollEvent.data.ptr;    \
			if (pTimeEvent) {    \
				if (pTimeEvent->m_funPrepare) {    \
					pTimeEvent->m_funPrepare();    \
				} else {    \
					AddActive(pTimeEvent);    \
				}    \
			}    \
		}    \
		DEBUG("can come here.");    \
		m_oTimer.GetAllTimeoutEvent(m_listActiveEvent);    \
		for (auto it = m_listActiveEvent.begin(); it != m_listActiveEvent.end(); ++it) {    \
			auto pTimeEvent = *it;    \
			if (pTimeEvent->m_funProcess) {    \
				pTimeEvent->m_funProcess();    \
				DEBUG("son swap out and turn to father");    \
			}    \
		}    \
		DEBUG("why stop..");    \
		m_listActiveEvent.clear();    \
	}    \
	if (pEpollEvents) {    \
		delete []pEpollEvents;    \
		pEpollEvents = nullptr;    \
	}    \
}


}
