#pragma once
#include "vector"
#include "coroutine.h"
#include "thread.h"
#include "cppsvrconfig.h"
#include "sys/epoll.h"
#include "timer.h"
#include "noncopyable.h"

namespace cppsvr {

// 协程池，每个线程里面只会有一个。
// 原来有 SO_REUSEPORT 这种东西，直接就允许多个（线程
// 的）fd 去监听同一个 ip-port，由内核来实现负载均衡。

class CoroutinePool {
	NON_COPY_ABLE(CoroutinePool);
public:
	CoroutinePool();
	virtual ~CoroutinePool();
	virtual void Run(bool bUseCaller = false);
	void AddActive(TimeEvent::ptr pTimeEvent);

	static CoroutinePool *GetThis();
	static void SetThis(CoroutinePool *pCurrentPool);
	// 事件默认的 prepare 和 process
	static void DefaultPrepare(TimeEvent::ptr pTimeEvent);
	static void DefaultProcess(Coroutine *pCoroutine);
	static void WaitFdEventWithTimeout(int iFd, int iEpollEvent, uint32_t iRelativeTimeout);

	// 如果失败或者超时了都自动关闭 fd。
	static int Read(int iFd, std::string &sMessage, uint32_t iRelativeTimeout = UINT32_MAX);
	static int Write(int iFd, std::string &sMessage, uint32_t iRelativeTimeout = UINT32_MAX);

protected:
	void AllCoroutineStart();
	// 注意每个子类的析构函数都要有这个东西，且放第一行！
	void WaitThreadRunEnd();
	// 子类自己设置各个协程要执行的函数
	virtual void InitCoroutines() = 0;

private:
	void ThreadRun();

protected:
	int m_iId;
	int m_iEpollFd;
	std::vector<Coroutine*> m_vecCoroutine;
	Thread* m_pThread;
	Timer m_oTimer;
	std::list<TimeEvent::ptr> m_listActiveEvent;
};


}
