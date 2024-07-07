#pragma once
#include "logger.h"
#include "commfunctions.h"
#include "thread.h"
#include "sys/epoll.h"
#include "sys/socket.h"
#include "coroutine.h"
#include "functional"
#include "unordered_map"

// 主 reactor 负责处理接受连接的事件，只要连接上了就把 socket 均匀分发给子 reactor（分给那个当前处理的连接数最少的一个）。
// 主线程就用作主 reactor。

// 子 reactor 负责处理主 reactor 交给它的连接，这些连接产生的事件就都注册到这个 reactor 上面，但是具体的业务任务可以由线程池中的
// 不同工作线程去完成。子 reactor 负责和多个工作线程交互，以快速完成一个业务，并且直接执行和客户端交流的操作，否则由于多个工作线
// 程同时读写一个 socket 会出问题。

namespace cppsvr {

class Reactor {
public:
	// 自定义简单读写事件枚举
	enum Event {
		NONE_EVENT = 0,
		READ_EVENT = EPOLLIN,
		WRITE_EVENT = EPOLLOUT,
		ERROR_EVENT = EPOLLERR,
	};
	
	Reactor(std::string sName = "");
	~Reactor();

	// 主循环
	void MainLoop();
	// 注册事件
	void RegisterEvent(int iFd, Event eEvent, std::function<void()> funCallBack, bool bRepeated = true);
	// 注销事件
	void UnregisterEvent(int iFd, Event eEvent);
	// 唤醒我
	void WakeUp();

	// 文件描述符上下文，绑定到 epoll 事件的指针上，用于注册回调函数。
	// 只有一个 reactor 会去实际操作这个东西，所以没必要锁。
	struct FdContext {
		FdContext(int iFd) : m_iFd(iFd), m_eEvent(NONE_EVENT), m_pReadHandler(nullptr),
					m_pWriteHandler(nullptr), m_pErrorHandler(nullptr) {}
		int m_iFd;
		Event m_eEvent;
		std::function<void()> m_pReadHandler;
		std::function<void()> m_pWriteHandler;
		std::function<void()> m_pErrorHandler;
	};
	
private:
	// 被唤醒后的操作。
	void WakeUpHandler();


public:
	int m_iPipeFds[2]; // 用于唤醒

private:
	std::string m_sName;
	int m_iEpollFd;
	std::unordered_map<int, FdContext> m_mapFd2FdContext;
};

Reactor::Event &operator|=(Reactor::Event&, Reactor::Event);
Reactor::Event &operator|=(Reactor::Event&, int);
Reactor::Event &operator&=(Reactor::Event&, Reactor::Event);
Reactor::Event &operator&=(Reactor::Event&, int);


}