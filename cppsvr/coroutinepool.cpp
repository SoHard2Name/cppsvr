#include "cppsvr/coroutinepool.h"
#include "cppsvr/commfunctions.h"
#include "atomic"
#include "cstring"

namespace cppsvr {


static std::atomic<uint32_t> g_iId{0};
thread_local CoroutinePool *t_pCurrentCoroutinePool = nullptr;

CoroutinePool::CoroutinePool() : m_iId(++g_iId), m_vecCoroutine(), m_pThread(nullptr), m_oTimer() {
	m_iEpollFd = epoll_create(1);
	assert(m_iEpollFd >= 0);
}

CoroutinePool::~CoroutinePool() {
	// 注意每个子类都要有这个东西，放第一个
	WaitThreadRunEnd();
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
	// INFO("???? here right?");
	pCoroutine->SwapIn();
}

void CoroutinePool::WaitFdEventWithTimeout(int iFd, int iEpollEvent, uint32_t iRelativeTimeout) {
	auto pTimeEvent = std::make_shared<TimeEvent>(GetCurrentTimeMs() + iRelativeTimeout,
						nullptr, std::bind(&DefaultProcess, Coroutine::GetThis()));
	pTimeEvent->m_funPrepare = std::bind(&DefaultPrepare, pTimeEvent);
	GetThis()->m_oTimer.AddTimeEvent(pTimeEvent);
	if (iFd != -1) {
		epoll_event oEpollEvent = {};
		oEpollEvent.events = iEpollEvent | EPOLLET;
		oEpollEvent.data.ptr = &pTimeEvent;
		int iRet = epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_ADD, iFd, &oEpollEvent);
		if (iRet) {
			ERROR("EPOLL_CTL_ADD error. errno %d, errmsg %s", errno, strerror(errno));
		}
	}
	// DEBUG("one coroutine will swap out and wait");
	Coroutine::GetThis()->SwapOut();
	// DEBUG("one routine swap in and continue");
	if (iFd != -1) {
		int iRet = epoll_ctl(GetThis()->m_iEpollFd, EPOLL_CTL_DEL, iFd, nullptr);
		if (iRet) {
			ERROR("EPOLL_CTL_DEL error. errno %d, errmsg %s", errno, strerror(errno));
		}
	}
}

// 每个 InitCoroutines 的函数末尾都要调且仅调用一次这个函数！
void CoroutinePool::AllCoroutineStart() {
	for (Coroutine *pCoroutine : m_vecCoroutine) {
		pCoroutine->SwapIn();
	}
}

void CoroutinePool::WaitThreadRunEnd() {
	if (m_pThread) {
		m_pThread->Join();
		delete m_pThread;
		m_pThread = nullptr;
	}
}

void CoroutinePool::Run(bool bUseCaller/* = false*/) {
	if (bUseCaller) {
		Thread::SetThreadName(StrFormat("CoroutunePool_%u_Thread_Use_Caller", m_iId));
		ThreadRun();
	} else {
		m_pThread = new Thread(std::bind(&CoroutinePool::ThreadRun, this), StrFormat("CoroutunePool_%u_Thread", m_iId));
	}
}

void CoroutinePool::InitLogReporterCoroutine() {
	m_vecCoroutine.push_back(new Coroutine(std::bind(&CoroutinePool::LogReporterCoroutine, this)));
}

void CoroutinePool::LogReporterCoroutine() {
	uint32_t iFlushInterval = CppSvrConfig::GetSingleton()->GetFlushInterval();
	Timer::GetThis()->AddRelativeTimeEvent(iFlushInterval, nullptr, std::bind(DefaultProcess, Coroutine::GetThis()), iFlushInterval);
	while (true) {
		DEBUG("PushLogToGlobalBuffer ...");
		Logger::PushLogToGlobalBuffer();
		Coroutine::GetThis()->SwapOut();
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
	DEBUG("init coroutines end. begin event loop...");
	const int iEventsSize = 1024, iWaitTimeMs = 100; // 毫秒级的定时器
	epoll_event *pEpollEvents = new epoll_event[iEventsSize];
	memset(pEpollEvents, 0, sizeof(epoll_event) * iEventsSize);
	while (true) {
		int iRet = epoll_wait(m_iEpollFd, pEpollEvents, iEventsSize, iWaitTimeMs);
		// DEBUG("epoll wait ret = %d", iRet);
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
		// DEBUG("end get time out, m_listActiveEvent size %zu", m_listActiveEvent.size());
		for (auto it = m_listActiveEvent.begin(); it != m_listActiveEvent.end(); ++it) {
			auto pTimeEvent = *it;
			if (pTimeEvent->m_funProcess) {
				pTimeEvent->m_funProcess();
			}
		}
		m_listActiveEvent.clear();
	}
	if (pEpollEvents) {
		delete []pEpollEvents;
		pEpollEvents = nullptr;
	}
}

static const int g_iBufferSize = 1024;
thread_local char *pBuffer = (char*)malloc(g_iBufferSize);

int CoroutinePool::Read(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	memset(pBuffer, 0, g_iBufferSize);
	bool bHasReceiveHead = false;
	uint32_t iMessageLen = 0;
	int iResult = -1; // -1 就说明超时了
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut) {
		int iRet = read(iFd, pBuffer, g_iBufferSize);
		// DEBUG("read iRet = %d", iRet);
		if (iRet < 0) {
			if (errno == EAGAIN) {
				// DEBUG("ret = -1 but errno is EAGAIN");
				CoroutinePool::GetThis()->WaitFdEventWithTimeout(iFd, EPOLLIN, iTimeOut - iNow);
				continue;
			} else {
				iResult = 1;
				break;
			}
		}
		// 连接已关闭
		if (iRet == 0) {
			iResult = 2;
			break;
		}
		// sMessage += pBuffer; // 由于 pBuffer[0] 很可能是 0，所以一定是错的，即使第一个不是 0，这样写也不行！！！中间有 0 还是寄掉
								// 同理，strlen 函数在这里也是无济于事的！！！
		sMessage += std::string(pBuffer, pBuffer + iRet);
		memset(pBuffer, 0, iRet);
		// 暂时定下协议是：4 字节消息体长度 + 消息体。（关于 req id 可以在消息体里面再定）
		std::string sLog;
		for (char c : sMessage) {
			sLog += "_" + std::to_string((int)c);
		}
		// DEBUG("sMessage: [%s], len %zu", sLog.c_str(), sMessage.size());
		if (bHasReceiveHead == false && sMessage.length() >= 4) {
			bHasReceiveHead = true;
			iMessageLen = ByteStr2UInt(sMessage.substr(0, 4));
			// DEBUG("iMessageLen: %u", iMessageLen);
			sMessage.erase(0, 4);
		}
		if (bHasReceiveHead == true && sMessage.length() >= iMessageLen) {
			iResult = 0;
			break;
		}
	}
	if (iResult) {
		close(iFd);
	}
	return iResult;
}

int CoroutinePool::Write(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	uint32_t iWrotenLen = 0;
	sMessage = UInt2ByteStr(sMessage.length()) + sMessage;
	int iResult = -1;
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut && iWrotenLen < sMessage.length()) {
		int iRet = write(iFd, sMessage.c_str() + iWrotenLen, sMessage.length() - iWrotenLen);
		// DEBUG("write iRet = %d", iRet);
		if (iRet < 0) {
			if (errno == EAGAIN) {
				// DEBUG("write fail but EAGAIN");
				CoroutinePool::GetThis()->WaitFdEventWithTimeout(iFd, EPOLLOUT, iTimeOut - iNow);
				continue;
			} else {
				ERROR("write fail finally. fd %d, errno %d, errmsg %s", iFd, errno, strerror(errno));
				iResult = 1;
				break;
			}
		} else if (iRet == 0) {
			iResult = 2;
			break;
		} else {
			iWrotenLen += iRet;
		}
	}
	if (iWrotenLen >= sMessage.length()) {
		// DEBUG("write succ! fd %d", iFd);
		return 0;
	}
	close(iFd);
	return iResult;
}


}