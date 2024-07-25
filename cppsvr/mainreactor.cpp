#include "mainreactor.h"
#include "sys/epoll.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "cppsvr/commfunctions.h"

// 睡了。。。

// 改造：配置、这里创建几个子响应器，然后友元类，然后 找连接数最少的 -> 添加（记
// 得给他计数器加） + 唤醒，然后子响应器那边也要把所谓 accept 改改。

// 改造第二阶段：日志器。

namespace cppsvr {

MainReactor::MainReactor(uint32_t iWorkerThreadNum/* = 配置数*/) : m_iListenFd(-1) {
	m_iListenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(m_iListenFd >= 0);
	SetNonBlock(m_iListenFd);
	auto &oCppSvrConfig = *CppSvrConfig::GetSingleton();
	std::string sIp = oCppSvrConfig.GetIp();
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(sIp.c_str()); // 这个表示监听主机的所有网卡
	addr.sin_port = htons(oCppSvrConfig.GetPort());
	if (oCppSvrConfig.GetReuseAddr()) {
		// 这个是设置它能重用地址，也就是上次关闭后可能还处于 time wait 的时候就能立即复用了。
		int iReuseaddrOpt = 1;
		assert(!setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEADDR, &iReuseaddrOpt, sizeof(iReuseaddrOpt)));
	}
	// // 这个是设置单个端口可以被多个 fd 绑定，这样不用让多个线程监听同一
	// // 个 fd 发生太多由于锁导致的线程切换，而是底层会自己分配 syn 请求。
	// int iReuseportOpt = 1;
	// assert(!setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEPORT, &iReuseportOpt, sizeof(iReuseportOpt)));
	assert(!bind(m_iListenFd, (struct sockaddr *)&addr, sizeof(addr)));
	INFO("bind succ. fd %d", m_iListenFd);
	assert(!listen(m_iListenFd, 128));
	INFO("listening... fd %d", m_iListenFd);
	
	for (int i = 0; i < iWorkerThreadNum; i++) {
		
	}
}

MainReactor::~MainReactor() {
	WaitThreadRunEnd();
	if (m_iListenFd >= 0) {
		close(m_iListenFd);
	}
}

void MainReactor::Run() {
	// 启动子响应器
	for (auto &oSubReactor : m_vecSubReactor) {
		oSubReactor.Run();
	}
	// 启动自己，并且用上当前线程
	CoroutinePool::Run(true);
}

void MainReactor::InitCoroutines() {
	// 初始化 accept 协程
	auto *pAcceptCoroutine = new Coroutine(std::bind(&MainReactor::AcceptCoroutine, this));
	m_vecCoroutine.push_back(pAcceptCoroutine);
	// 初始化推日志到磁盘的协程（它也是把公共区域拿到本线程的区域然后再来处理）
	
	
	AllCoroutineStart();
}

void MainReactor::AcceptCoroutine() {
	// WARN("in accept coroutine, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
	while (true) {
		// WARN("in accept coroutine pos 2, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
		// static int iTestCount = 0;
		// INFO("TEST: accept coroutine running, tick %d", iTestCount++);
		int iFd = accept(m_iListenFd, nullptr, nullptr);
		if (iFd < 0) {
			INFO("ret %d, errno %d, errmsg %s", iFd, errno, strerror(errno));
			CoroutinePool::GetThis()->WaitFdEventWithTimeout(m_iListenFd, EPOLLIN, 1000);
			// WARN("in accept coroutine pos 3, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
			continue;
		}
		INFO("conn succ. fd %d", iFd);
		SetNonBlock(iFd);
		INFO("core ??? 1");
		m_listFd.push_back(iFd);
		INFO("core ??? 2");
		WARN("in accept coroutine pos 4, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
		m_oCoSemaphore.Post();
		INFO("core ??? 3");
	}
}

}