#include "cppsvr/coroutinepool.h"
#include "cppsvr/commfunctions.h"
#include "atomic"
#include "cstring"

namespace cppsvr {


static std::atomic<uint32_t> g_iId{0};
thread_local CoroutinePool *t_pCurrentCoroutinePool = nullptr;

CoroutinePool::CoroutinePool(uint32_t iCoroutineNum/* = 配置的协程数*/) : m_iId(++g_iId), m_iCoroutineNum(iCoroutineNum),
		m_vecCoroutine(iCoroutineNum, nullptr), m_pThread(nullptr), m_oTimer() {
	m_iEpollFd = epoll_create(1);
	assert(m_iEpollFd >= 0);
}

CoroutinePool::~CoroutinePool() {
	// 注意每个子类都要有这个东西，放第一个
	if (m_pThread) {
		m_pThread->Join();
		delete m_pThread;
		m_pThread = nullptr;
	}
	if (m_iEpollFd >= 0) {
		close(m_iEpollFd);
	}
	for (auto &pCoroutine : m_vecCoroutine) {
		if (pCoroutine) {
			delete pCoroutine;
			pCoroutine = nullptr;
		}
	}
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
	INFO("???? here right?");
	pCoroutine->SwapIn();
}

void CoroutinePool::WaitFdEventWithTimeout(int iFd, int iEpollEvent, uint32_t iRelativeTimeout) {
	auto pTimeEvent = std::make_shared<TimeEvent>(GetCurrentTimeMs() + iRelativeTimeout,
						nullptr, std::bind(&DefaultProcess, Coroutine::GetThis()));
	pTimeEvent->m_funPrepare = std::bind(&DefaultPrepare, pTimeEvent);
	GetThis()->m_oTimer.AddTimeEvent(pTimeEvent);
	DEBUG("TEST: pTimeEvent belong list %d", pTimeEvent->m_iBelongList);
	if (iFd != -1) {
		epoll_event oEpollEvent = {};
		oEpollEvent.events = iEpollEvent | EPOLLET;
		oEpollEvent.data.ptr = &pTimeEvent;
		INFO("HERE?!!!!!!!");
		int iRet = epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_ADD, iFd, &oEpollEvent);
		if (iRet) {
			ERROR("EPOLL_CTL_ADD error. errno %d, errmsg %s", errno, strerror(errno));
		}
	}
	DEBUG("TEST: one coroutine will swap out and wait");
	Coroutine::GetThis()->SwapOut();
	INFO("TEST: one routine swap in and continue");
	if (iFd != -1) {
		int iRet = epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_DEL, iFd, nullptr);
		if (iRet) {
			ERROR("EPOLL_CTL_DEL error. errno %d, errmsg %s", errno, strerror(errno));
		}
	}
}

void CoroutinePool::InitCoroutines() {
	assert(m_iCoroutineNum >= 1);
	// 初始化日志协程，专门每隔一段时间就把缓冲区内容弄
	// 到全局，全局缓冲区内容会由主线程不断输出到文件。
	
}

void CoroutinePool::Run(bool bUseCaller/* = false*/) {
	if (bUseCaller) {
		Thread::SetThreadName(StrFormat("CoroutunePool_%u_Thread_Use_Caller", m_iId));
		ThreadRun();
	} else {
		m_pThread = new Thread(std::bind(&CoroutinePool::ThreadRun, this), StrFormat("CoroutunePool_%u_Thread", m_iId));
	}
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
	INFO("init coroutines end. begin event loop...");
	const int iEventsSize = 1024, iWaitTimeMs = 100; // 毫秒级的定时器
	epoll_event *pEpollEvents = new epoll_event[iEventsSize];
	memset(pEpollEvents, 0, sizeof(epoll_event) * iEventsSize);
	while (true) {
		// std::cout << "??? \n";
		int iRet = epoll_wait(m_iEpollFd, pEpollEvents, iEventsSize, iWaitTimeMs);
		// INFO("epoll wait ret = %d", iRet);
		// if (iRet != 0) {
		// 	std::cout << "iRet is not zero: " << iRet << std::endl;
		// }
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
		DEBUG("can come here.");
		m_oTimer.GetAllTimeoutEvent(m_listActiveEvent);
		// INFO("end get time out, m_listActiveEvent size %zu", m_listActiveEvent.size());
		// if (m_listActiveEvent.size() > 0) {
		// 	std::cout << "m_listActiveEvent size > 0 : " << m_listActiveEvent.size() << std::endl;
		// }
		for (auto it = m_listActiveEvent.begin(); it != m_listActiveEvent.end(); ++it, INFO("???... it %p", it->get()) ) {
			INFO("wht....");
			auto pTimeEvent = *it;
			INFO("wht....333");
			if (pTimeEvent->m_funProcess) {
				INFO("wht....6666");
				pTimeEvent->m_funProcess();
				INFO("son swap out and turn to father");
			}
			INFO("到这里是没问题的。。pTimeEvent use count %ld", pTimeEvent.use_count());
		}
		INFO("why stop..");
		m_listActiveEvent.clear();
		INFO("why stop..2");
	}
	if (pEpollEvents) {
		delete []pEpollEvents;
		pEpollEvents = nullptr;
	}
}

}