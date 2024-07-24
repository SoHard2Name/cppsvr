#include "cppsvr/subreactor.h"
#include "cppsvr/commfunctions.h"
#include "cstring"

namespace cppsvr {

// 初始化。
std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> SubReactor::g_mapId2Service;

SubReactor::SubReactor(uint32_t iCoroutineNum/* = 配置数*/) : 
		CoroutinePool(iCoroutineNum), m_queFd(), m_oCoSemaphore(0) {
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
	// 这个是设置单个端口可以被多个 fd 绑定，这样不用让多个线程监听同一
	// 个 fd 发生太多由于锁导致的线程切换，而是底层会自己分配 syn 请求。
	int iReuseportOpt = 1;
	assert(!setsockopt(m_iListenFd, SOL_SOCKET, SO_REUSEPORT, &iReuseportOpt, sizeof(iReuseportOpt)));
	assert(!bind(m_iListenFd, (struct sockaddr *)&addr, sizeof(addr)));
	INFO("bind succ. fd %d", m_iListenFd);
	assert(!listen(m_iListenFd, 128));
	INFO("listening... fd %d", m_iListenFd);
}

SubReactor::~SubReactor() {
	// 每个继承于 CoroutinePool 的类的析构里面都应该有这个东西！！！
	// 并且放第一个，否则属于子类的东西就被销毁了，虚函数什么的就乱了，因为虚表被销毁了。
	MUST_WAIT_THREAD_IN_EVERY_SON_CLASS_DESTRCUTOR_FIRST_LINE
	if (m_iListenFd >= 0) {
		close(m_iListenFd);
	}
	while (m_queFd.size()) {
		int iFd = m_queFd.front();
		if (iFd >= 0) {
			close(iFd);
		}
	}
}

void SubReactor::InitCoroutines() {
	INFO("begin InitCoroutines ... ");
	// assert(0);
	CoroutinePool::InitCoroutines();
	assert(m_iCoroutineNum >= 3);
	// 初始化 accept 协程
	auto &pAcceptCoroutine = m_vecCoroutine[1];
	pAcceptCoroutine = new Coroutine(std::bind(&SubReactor::AcceptCoroutine, this));
	pAcceptCoroutine->SwapIn();
	// 剩下的都是 read write 协程，里面也处理业务
	for (int i = 2; i < m_iCoroutineNum; i++) {
		m_vecCoroutine[i] = new Coroutine(std::bind(&SubReactor::ReadWriteCoroutine, this));
		m_vecCoroutine[i]->SwapIn();
	}
}

void SubReactor::AcceptCoroutine() {
	WARN("in accept coroutine, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
	while (true) {
		WARN("in accept coroutine pos 2, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
		static int iTestCount = 0;
		INFO("TEST: accept coroutine running, tick %d", iTestCount++);
		int iFd = accept(m_iListenFd, nullptr, nullptr);
		if (iFd < 0) {
			INFO("ret %d, errno %d, errmsg %s", iFd, errno, strerror(errno));
			CoroutinePool::GetThis()->WaitFdEventWithTimeout(m_iListenFd, EPOLLIN, 1000);
			WARN("in accept coroutine pos 3, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
			continue;
		}
		INFO("conn succ. fd %d", iFd);
		SetNonBlock(iFd);
		INFO("core ??? 1");
		m_queFd.push(iFd);
		INFO("core ??? 2");
		WARN("in accept coroutine pos 4, this %p CoSemaphore count %d", this, m_oCoSemaphore.GetCount());
		m_oCoSemaphore.Post();
		INFO("core ??? 3");
	}
}

void SubReactor::ReadWriteCoroutine() {
	// DEBUG("one ReadWriteCoroutine be init");
	const int iBufferSize = 1024;
	char *pBuffer = (char*)malloc(iBufferSize);
	memset(pBuffer, 0, iBufferSize);
	while (true) {
		m_oCoSemaphore.Wait();
		int iFd = m_queFd.front();
		m_queFd.pop();
		// TODO: 改成根据连接超时来弄循环，这样能关闭那些太久没有通信的连接。如果有交流则更新超时时间。
		while (true) {
			DEBUG("TEST: one service.");
			std::string sReq;
			int iRet = Read(iFd, sReq);
			if (iRet != 0) {
				ERROR("read req error. ret %d, fd %d", iRet, iFd);
				break;
			}
			// 暂时定下协议是：4 字节服务编号、然后是消息。
			// TODO: 服务 id 弄成枚举值，配合 protobuf
			uint32_t iServiceId = ByteStr2UInt(sReq.substr(0, 4));
			sReq.erase(0, 4);
			auto it = g_mapId2Service.find(iServiceId);
			if (it == g_mapId2Service.end()) {
				ERROR("no such service. service id %u", iServiceId);
			} else {
				std::string sResp;
				it->second(sReq, sResp);
				int iRet = Write(iFd, sResp);
				if (iRet) {
					ERROR("read req error. ret %d", iRet);
					break;
				}
			}
		}
	}
	if (pBuffer != nullptr) {
		std::cout << "这里来两次？？？" << std::endl;
		free(pBuffer);
		pBuffer = nullptr;
	}
}

static const int g_iBufferSize = 1024;
thread_local char *pBuffer = (char*)malloc(g_iBufferSize);

int SubReactor::Read(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	memset(pBuffer, 0, g_iBufferSize);
	bool bHasReceiveHead = false;
	uint32_t iMessageLen = 0;
	int iResult = -1; // -1 就说明超时了
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut) {
		int iRet = read(iFd, pBuffer, g_iBufferSize);
		DEBUG("read iRet = %d", iRet);
		if (iRet < 0) {
			if (errno == EAGAIN) {
				DEBUG("ret = -1 but is here");
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
		// TODO: 消息正确性有 tcp 包着了，自己没必要再验证。
		std::string sLog;
		for (char c : sMessage) {
			sLog += "_" + std::to_string((int)c);
		}
		DEBUG("sMessage: [%s], len %zu", sLog.c_str(), sMessage.size());
		if (bHasReceiveHead == false && sMessage.length() >= 4) {
			bHasReceiveHead = true;
			iMessageLen = ByteStr2UInt(sMessage.substr(0, 4));
			DEBUG("iMessageLen: %u", iMessageLen);
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

int SubReactor::Write(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	// std::cout << "你们在干啥。。" << std::endl;
	uint32_t iWrotenLen = 0;
	sMessage = UInt2ByteStr(sMessage.length()) + sMessage;
	int iResult = -1;
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut && iWrotenLen < sMessage.length()) {
		int iRet = write(iFd, sMessage.c_str() + iWrotenLen, sMessage.length() - iWrotenLen);
		INFO("write iRet = %d", iRet);
		if (iRet < 0) {
			if (errno == EAGAIN) {
				DEBUG("write fail and EAGAIN...");
				CoroutinePool::GetThis()->WaitFdEventWithTimeout(iFd, EPOLLOUT, iTimeOut - iNow);
				continue;
			} else {
				ERROR("write fail finally.. fd %d, errno %d, errmsg %s", iFd, errno, strerror(errno));
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
		DEBUG("write succ! fd %d", iFd);
		return 0;
	}
	close(iFd);
	return iResult;
}

void SubReactor::RegisterService(uint32_t iServiceId, std::function<void(const std::string &, std::string &)> funService) {
	g_mapId2Service[iServiceId] = std::move(funService);
}

}
