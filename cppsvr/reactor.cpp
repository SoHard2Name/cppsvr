#include "cppsvr/reactor.h"
#include "cstring"
#include "fcntl.h"

namespace cppsvr {

Reactor::Event& operator|=(Reactor::Event& eLEvent, Reactor::Event eREvent) {
	return eLEvent = (Reactor::Event)(eLEvent | eREvent);
}

Reactor::Event& operator|=(Reactor::Event& eLEvent, int iREvent) {
	return eLEvent = (Reactor::Event)(eLEvent | iREvent);
}

Reactor::Event& operator&=(Reactor::Event& eLEvent, Reactor::Event eREvent) {
	return eLEvent = (Reactor::Event)(eLEvent & eREvent);
}

Reactor::Event& operator&=(Reactor::Event& eLEvent, int iREvent) {
	return eLEvent = (Reactor::Event)(eLEvent & iREvent);
}

Reactor::Reactor(std::string sName/* = ""*/) : m_sName(sName) {
	m_iEpollFd = epoll_create(1);
	if (m_iEpollFd <= 0) {
		ERROR("create epoll err. epfd %d", m_iEpollFd);
		exit(0);
	}
	
	int iRet = pipe(m_iPipeFds);
	if (iRet) {
		ERROR("create pipe err. ret %d", iRet);
		exit(0);
	}
	iRet = fcntl(m_iPipeFds[0], F_SETFL, O_NONBLOCK);
	if (iRet) {
		ERROR("set non block flag err. ret %d", iRet);
		exit(0);
	}
	
	RegisterEvent(m_iPipeFds[0], READ_EVENT, std::move(std::bind(&Reactor::WakeUpHandler, this)));
}

Reactor::~Reactor() {
	close(m_iEpollFd);
	close(m_iPipeFds[0]);
	close(m_iPipeFds[1]);
}

void Reactor::MainLoop() {
	const int iEventsSize = 64;
	epoll_event oEpollEvents[iEventsSize] = {};
	const int iMaxWaitMs = 5000;
	while (true) {
		DEBUG("reactor %s MainLoop one new round", m_sName.c_str());
		int iRet = epoll_wait(m_iEpollFd, oEpollEvents, iEventsSize, iMaxWaitMs);
		if (iRet < 0) {
			if (errno != EINTR) {
				ERROR("epoll_wait error. ret %d", iRet);
			}
			continue;
		}
		const int iEventNum = iRet;
		for (int i = 0; i < iEventNum; i++) {
			auto &oEpollEvent = oEpollEvents[i];
			auto *pFdCtx = (FdContext*)oEpollEvent.data.ptr;
			auto iTriggeredEvent = oEpollEvent.events;
			if (pFdCtx) {
				if (iTriggeredEvent & READ_EVENT) { // 触发读事件
					(pFdCtx->m_pReadHandler)();
				}
				if (iTriggeredEvent & WRITE_EVENT) { // 触发写事件
					(pFdCtx->m_pWriteHandler)();
				}
				if (iTriggeredEvent & ERROR_EVENT) { // 触发错误事件。
					ERROR("fd %d has error event.", pFdCtx->m_iFd);
					(pFdCtx->m_pErrorHandler)();
				}
			} else {
				ERROR("fd ctx is null.");
			}
		}
	}
	ERROR("main loop end. name %s", m_sName.c_str());
}

void Reactor::RegisterEvent(int iFd, Event eEvent, std::function<void()> funCallBack, bool bRepeated/* = true*/){
	// 这样就能做到每次调用的时候是一次原本注册的那个回调函数了
	// 这里如果还是用 '&' 就将来要用到的时候一堆变量其实都被清理了。
	auto funRealCallBack = [=](){
		funCallBack();
		if (bRepeated == false) {
			UnregisterEvent(iFd, eEvent);
		}
	};

	if (m_mapFd2FdContext.count(iFd) == 0) {
		m_mapFd2FdContext.emplace(iFd, iFd);
	}
	// 不能直接 m_mapFd2FdContext[iFd] 因为这种需要无参构造函数。
	auto &oFdCtx = m_mapFd2FdContext.find(iFd)->second;

#define SET_HANDLER(EventType) \
		if (oFdCtx.m_p##EventType##Handler != nullptr) { \
			WARN("fd %d already has " #EventType " handler", iFd); \
		} \
		oFdCtx.m_p##EventType##Handler = funRealCallBack

	if (eEvent == READ_EVENT) {
		SET_HANDLER(Read);
	} else if (eEvent == WRITE_EVENT) {
		SET_HANDLER(Write);
	} else if (eEvent == ERROR_EVENT) {
		SET_HANDLER(Error);
	} else {
		ERROR("unlegal event %d", eEvent);
		return;
	}

#undef SET_HANDLER

	int iEpollCtlOp = oFdCtx.m_eEvent ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
	epoll_event oEpollEvent = {};
	oEpollEvent.events = EPOLLET | (oFdCtx.m_eEvent |= eEvent);
	oEpollEvent.data.ptr = &oFdCtx;
	int iRet = epoll_ctl(m_iEpollFd, iEpollCtlOp, iFd, &oEpollEvent);
	if (iRet) {
		ERROR("epoll_ctl fail. ret %d", iRet);
	}
}

void Reactor::UnregisterEvent(int iFd, Event eEvent){
	if (m_mapFd2FdContext.count(iFd) == 0) {
		WARN("do not have this fd. fd %d, event %d", iFd, eEvent);
		return;
	}
	auto &oFdCtx = m_mapFd2FdContext.find(iFd)->second;

#define REMOVE_HANDLER(EventType) \
		if (oFdCtx.m_p##EventType##Handler == nullptr) { \
			WARN(#EventType " hander is null. fd %d, event %d", iFd, eEvent); \
			return; \
		} \
		oFdCtx.m_p##EventType##Handler = nullptr
		
	if (eEvent == READ_EVENT) {
		REMOVE_HANDLER(Read);
	} else if (eEvent == WRITE_EVENT) {
		REMOVE_HANDLER(Write);
	} else if (eEvent == ERROR_EVENT) {
		REMOVE_HANDLER(Error);
	} else {
		ERROR("unlegal event %d", eEvent);
		return;
	}

#undef REMOVE_HANDLER

	int iEpollCtlOp = oFdCtx.m_eEvent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
	epoll_event oEpollEvent = {};
	oEpollEvent.events = EPOLLET | (oFdCtx.m_eEvent &= ~eEvent);
	oEpollEvent.data.ptr = &oFdCtx;
	int iRet = epoll_ctl(m_iEpollFd, iEpollCtlOp, iFd, &oEpollEvent);
	if (iRet) {
		ERROR("epoll_ctl fail. ret %d", iRet);
	}
	if (oFdCtx.m_eEvent == NONE_EVENT) {
		m_mapFd2FdContext.erase(iFd);
	}
}

void Reactor::WakeUp() {
	int iRet = write(m_iPipeFds[1], "T", 1);
	if (iRet != 1) {
		ERROR("wake up error. ret %d", iRet);
	}
	DEBUG("wake up reactor %s", m_sName.c_str());
}

void Reactor::WakeUpHandler() {
	DEBUG("reactor %s has been waken up.", m_sName.c_str());
	char buf[8];
	while(read(m_iPipeFds[0], buf, 8) > 0); // 清空内容。
}

}