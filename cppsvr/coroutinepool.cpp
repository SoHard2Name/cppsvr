#include "cppsvr/coroutinepool.h"
#include "cppsvr/commfunctions.h"
#include "atomic"
#include "cstring"
#include "coroutinepool.h"

namespace cppsvr {

static std::atomic<uint32_t> g_iId{0};
thread_local CoroutinePool *t_pCurrentCoroutinePool = nullptr;

CoroutinePool::CoroutinePool(uint32_t iCoroutineNum/* = 配置的协程数*/) : m_iId(++g_iId), m_iCoroutineNum(iCoroutineNum),
		m_vecCoroutine(iCoroutineNum, nullptr), m_pThread(nullptr), m_oTimer() {
	m_iEpollFd = epoll_create(1);
	assert(m_iEpollFd >= 0);
}

CoroutinePool::~CoroutinePool() {
	if (m_pThread) {
		m_pThread->Join();
		delete m_pThread;
	}
	if (m_iEpollFd >= 0) {
		close(m_iEpollFd);
	}
	for (auto pCoroutine : m_vecCoroutine) {
		if (pCoroutine) {
			delete pCoroutine;
		}
	}
}

void CoroutinePool::Run() {
	m_pThread = new Thread(std::bind(&ThreadRun, this), cppsvr::StrFormat("CoroutunePool_%u_Thread", m_iId));
}

void CoroutinePool::AddActive(TimeEvent::ptr pTimeEvent) {
	m_listActiveEvent.push_back(pTimeEvent);
}

CoroutinePool *CoroutinePool::GetThis() {
	return t_pCurrentCoroutinePool;
}

void CoroutinePool::SetThis(CoroutinePool *pCurrentPool) {
	t_pCurrentCoroutinePool = pCurrentPool;
}

void CoroutinePool::DefaultPrepare(TimeEvent::ptr pTimeEvent) {
	// 从定时器上注销自己然后把自己加到 active 列表
	Timer::GetThis()->DeleteTimeEvent(pTimeEvent);
	GetThis()->AddActive(pTimeEvent);
}

void CoroutinePool::DefaultProcess(Coroutine* pCoroutine) {
	pCoroutine->SwapIn();
}

void CoroutinePool::WaitFdEventWithTimeout(int iFd, int iEpollEvent, uint32_t iRelativeTimeout) {
	epoll_event oEpollEvent = {};
	oEpollEvent.events = iEpollEvent | EPOLLET;
	auto pTimeEvent = std::make_shared<TimeEvent>(GetCurrentTimeMs() + iRelativeTimeout,
						nullptr, std::bind(&DefaultProcess, Coroutine::GetThis()));
	pTimeEvent->m_funPrepare = std::bind(&DefaultPrepare, pTimeEvent);
	GetThis()->m_oTimer.AddTimeEvent(pTimeEvent);
	oEpollEvent.data.ptr = &pTimeEvent;
	epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_ADD, iFd, &oEpollEvent);
	Coroutine::GetThis()->SwapOut();
	epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_DEL, iFd, nullptr);
}

void CoroutinePool::InitCoroutines() {
	assert(m_iCoroutineNum >= 1);
	// 初始化日志协程，专门每隔一段时间就把缓冲区内容弄
	// 到全局，全局缓冲区内容会由主线程不断输出到文件。
	
}

void CoroutinePool::ThreadRun() {
	// 设置线程变量
	assert(!GetThis());
	SetThis(this);
	assert(!Timer::GetThis());
	Timer::SetThis(&m_oTimer);
	// 初始化各个协程。
	InitCoroutines();
	// 事件循环
	const int iEventsSize = 1024, iWaitTimeMs = 1; // 毫秒级的定时器
	epoll_event *pEpollEvents = new epoll_event[iEventsSize];
	memset(pEpollEvents, 0, sizeof(epoll_event) * iEventsSize);
	while (true) {
		int iRet = epoll_wait(m_iEpollFd, pEpollEvents, iEventsSize, iWaitTimeMs);
		if (iRet < 0) {
			if (errno != EINTR) {
				ERROR("epoll_wait error. ret %d", iRet);
				break;
			}
			continue;
		}
		const int iEventNum = iRet;
		for (int i = 0; i < iEventNum; i++) {
			auto &oEpollEvent = pEpollEvents[i];
			auto pTimeEvent = *(TimeEvent::ptr*)oEpollEvent.data.ptr;
			if (pTimeEvent) {
				if (pTimeEvent->m_funPrepare) {
					pTimeEvent->m_funPrepare();
				} else {
					AddActive(pTimeEvent);
				}
			}
		}
		m_oTimer.GetAllTimeoutEvent(m_listActiveEvent);
		for (auto it = m_listActiveEvent.begin(); it != m_listActiveEvent.end(); ++it) {
			auto pTimeEvent = *it;
			if (pTimeEvent->m_funProcess) {
				pTimeEvent->m_funProcess();
			}
		}
		m_listActiveEvent.clear();
	}
	delete []pEpollEvents;
}

}