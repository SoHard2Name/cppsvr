#pragma once
#include "coroutinepool.h"
#include "cppsvrconfig.h"
#include "comutex.h"
#include "queue"
#include "unordered_map"
#include "string"
#include "sys/epoll.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "vector"
#include "atomic"
#include "mutex.h"

namespace cppsvr {

// 子响应器有多个，负责处理连接，跟客户端交互。
// 其中一个协程定期把日志从本线程缓冲区输送到全局缓冲区，
// 本线程实际打印日志都只是打到这个线程缓冲区上。
// 还有一个线程专门监听一个管道，当被唤醒之后就把连接加入
// 到任务队列中，然后 post 唤醒等待的工作协程。
// 剩下的是工作线程。

class SubReactor : public CoroutinePool {
	NON_COPY_ABLE(SubReactor);
public:
	SubReactor(uint32_t iWorkerCoroutineNum = CppSvrConfig::GetSingleton()->GetWorkerCoroutineNum());
	~SubReactor();
	virtual void InitCoroutines() override;

	// 这个保证是在单线程情况下进行。
	static void RegisterService(uint32_t iServiceId, std::function<void(const std::string&, std::string&)> funService);
	
	void AddFd(int iFd);
	int GetConnectNum();
	
private:
	// 每个函数都是一进一出，进表示 req，出表示 resp。
	// 每个服务器只需要通过在这个 map 上面注册服务就行，就是实现的时候还是要会使用本框架实现的 Read 函数之类。
	static std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> g_mapId2Service;
	
	int TransferFds();
	void WorkerCoroutine();
	void TransferFdsAndWakeUpWorkerCoroutine();

private:
	uint32_t m_iWorkerCoroutineNum;
	int m_iPipeFds[2];
	std::atomic<int> m_iConnectNum;
	std::list<int> m_listFdBuffer;
	std::list<int> m_listFd;
	CoSemaphore m_oCoSemaphore;
	// 因为明确获取锁后都是 O(1) 的简单操作，很少会冲突且很快会结束，所以用了自旋锁
	SpinLock m_oFdBufferMutex;
};

}
