#include "cppsvr/mainreactor.h"
#include "sys/epoll.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "cppsvr/commfunctions.h"

namespace cppsvr {

MainReactor::MainReactor(uint32_t iWorkerThreadNum/* = 配置数*/, 
		uint32_t iWorkerCoroutineNum/* = 配置数*/) : m_iListenFd(-1) {

	m_iListenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(m_iListenFd >= 0);
	SetNonBlock(m_iListenFd);
	auto &oCppSvrConfig = *CppSvrConfig::GetSingleton();
	std::string sIp = oCppSvrConfig.GetIp();
	struct sockaddr_in oAddr;
	oAddr.sin_family = AF_INET;
	oAddr.sin_addr.s_addr = inet_addr(sIp.c_str()); // 这个表示监听主机的所有网卡
	oAddr.sin_port = htons(oCppSvrConfig.GetPort());
	if (oCppSvrConfig.GetReuseAddr()) {
		// 这个是设置它能重用地址，也就是上次关闭后可能还处于 time wait 的时候就能立即复用了。
		int iReuseaddrOpt = 1;
		assert(!setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEADDR, &iReuseaddrOpt, sizeof(iReuseaddrOpt)));
	}
	assert(!bind(m_iListenFd, (struct sockaddr *)&oAddr, sizeof(oAddr)));
	INFO("bind succ. fd %d", m_iListenFd);
	assert(!listen(m_iListenFd, 128));
	INFO("listening... fd %d", m_iListenFd);

	for (int i = 0; i < iWorkerThreadNum; i++) {
		m_vecSubReactor.push_back(new SubReactor(iWorkerCoroutineNum));
	}
}

MainReactor::~MainReactor() {
	WaitThreadRunEnd();
	if (m_iListenFd >= 0) {
		close(m_iListenFd);
	}
	for (auto *pSubReactor : m_vecSubReactor) {
		if (pSubReactor) {
			delete pSubReactor;
			pSubReactor = nullptr;
		}
	}
}

void MainReactor::Run(bool bUseCaller/* = false*/) {
	// 启动子响应器
	DEBUG("start subreactor..");
	for (auto *pSubReactor : m_vecSubReactor) {
		pSubReactor->Run();
	}
	DEBUG("start mainreactor..");
	CoroutinePool::Run(bUseCaller);
}

void MainReactor::InitCoroutines() {
	// 初始化 accept 协程
	m_vecCoroutine.push_back(new Coroutine(std::bind(&MainReactor::AcceptCoroutine, this)));
	// 初始化推日志到磁盘的协程（它也是把公共区域拿到本线程的区域然后再来处理）
	InitStoreLogCoroutine();
	AllCoroutineStart();
}

void MainReactor::AcceptCoroutine() {
	while (true) {
		std::string sLog;
		for (int i = 0; i < m_vecSubReactor.size(); i++) {
			sLog += StrFormat("%d,%d;", i, m_vecSubReactor[i]->GetConnectNum());
		}
		INFO("sub reactor conn status: %s", sLog.c_str());
		int iFd = accept(m_iListenFd, nullptr, nullptr);
		if (iFd < 0) {
			// DEBUG("ret %d, errno %d, errmsg %s", iFd, errno, strerror(errno));
			CoroutinePool::GetThis()->WaitFdEventWithTimeout(m_iListenFd, EPOLLIN, 1000);
			continue;
		}
		INFO("conn succ. fd %d", iFd);
		SetNonBlock(iFd);
		int iMin = INT32_MAX, iIndex = -1;
		for (int i = 0; i < m_vecSubReactor.size(); i++) {
			int iConnectNum = m_vecSubReactor[i]->GetConnectNum();
			if (iConnectNum < iMin) {
				iMin = iConnectNum;
				iIndex = i;
			}
		}
		m_vecSubReactor[iIndex]->AddFd(iFd);
	}
}

}