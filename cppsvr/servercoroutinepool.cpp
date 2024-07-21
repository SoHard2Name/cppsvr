#include "cppsvr/servercoroutinepool.h"
#include "cppsvr/commfunctions.h"
#include "sys/epoll.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "cstring"

namespace cppsvr {

// 初始化。
std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> ServerCoroutinePool::g_mapId2Service;

ServerCoroutinePool::ServerCoroutinePool(uint32_t iCoroutineNum/* = 配置数*/) : 
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
	assert(!listen(m_iListenFd, 128));
}

void ServerCoroutinePool::InitCoroutines() {
	CoroutinePool::InitCoroutines();
	assert(m_iCoroutineNum >= 3);
	// 初始化 accept 协程
	auto &pAcceptCoroutine = m_vecCoroutine[1];
	pAcceptCoroutine = new Coroutine(std::bind(&ServerCoroutinePool::AcceptCoroutine, this));
	pAcceptCoroutine->SwapIn();
	// 剩下的都是 read write 协程，里面也处理业务
	for (int i = 2; i < m_iCoroutineNum; i++) {
		m_vecCoroutine[i] = new Coroutine(std::bind(&ServerCoroutinePool::ReadWriteCoroutine, this));
		m_vecCoroutine[i]->SwapIn();
	}
}

void ServerCoroutinePool::AcceptCoroutine() {
	while (true) {
		static int iTestCount = 0;
		DEBUG("TEST: accept coroutine running, tick %d", iTestCount++);
		int iFd = accept(m_iListenFd, nullptr, nullptr);
		if (iFd < 0) {
			CoroutinePool::GetThis()->WaitFdEventWithTimeout(m_iListenFd, EPOLLIN, 1000);
			continue;
		}
		SetNonBlock(iFd);
		m_queFd.push(iFd);
		m_oCoSemaphore.Post();
	}
}

void ServerCoroutinePool::ReadWriteCoroutine() {
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
				ERROR("read req error. ret %d", iRet);
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
		free(pBuffer);
	}
}

static const int g_iBufferSize = 1024;
thread_local char *pBuffer = (char*)malloc(g_iBufferSize);

int ServerCoroutinePool::Read(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	memset(pBuffer, 0, g_iBufferSize);
	bool bHasReceiveHead = false;
	uint32_t iMessageLen = 0;
	int iResult = -1; // -1 就说明超时了
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut) {
		int iRet = read(iFd, pBuffer, g_iBufferSize);
		if (iRet < 0) {
			if (errno == EAGAIN) {
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
		sMessage += pBuffer;
		memset(pBuffer, 0, iRet);
		// 暂时定下协议是：4 字节消息体长度 + 消息体。（关于 req id 可以在消息体里面再定）
		// TODO: 消息正确性有 tcp 包着了，自己没必要再验证。
		if (bHasReceiveHead == false && sMessage.length() >= 4) {
			bHasReceiveHead = true;
			iMessageLen = ByteStr2UInt(sMessage.substr(0, 4));
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

int ServerCoroutinePool::Write(int iFd, std::string &sMessage, uint32_t iRelativeTimeout/* = -1*/) {
	uint32_t iWrotenLen = 0;
	sMessage = UInt2ByteStr(sMessage.length()) + sMessage;
	int iResult = -1;
	uint64_t iNow = GetCurrentTimeMs(), iTimeOut = iNow + iRelativeTimeout;
	while ((iNow = GetCurrentTimeMs()) < iTimeOut && iWrotenLen < sMessage.length()) {
		int iRet = write(iFd, sMessage.c_str() + iWrotenLen, sMessage.length() - iWrotenLen);
		if (iRet < 0) {
			if (errno == EAGAIN) {
				CoroutinePool::GetThis()->WaitFdEventWithTimeout(iFd, EPOLLOUT, iTimeOut - iNow);
				continue;
			} else {
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
		return 0;
	}
	close(iFd);
	return iResult;
}

}
