#include "cppsvr/subreactor.h"
#include "cppsvr/commfunctions.h"
#include "cstring"
#include "subreactor.h"

namespace cppsvr {

// 初始化。
std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> SubReactor::g_mapId2Service;

SubReactor::SubReactor(uint32_t iWorkerCoroutineNum/* = 配置数*/) : m_iWorkerCoroutineNum(iWorkerCoroutineNum), 
		m_iConnectNum(0), m_listFdBuffer(), m_listFd(), m_oCoSemaphore(0) {
	memset(m_iPipeFds, -1, sizeof(m_iPipeFds));
	assert(!pipe(m_iPipeFds));
	SetNonBlock(m_iPipeFds[1]);
}

SubReactor::~SubReactor() {
	// 每个继承于 CoroutinePool 的类的析构里面都应该有这个东西！！！
	// 并且放第一个，否则属于子类的东西就被销毁了，虚函数什么的就乱了，因为虚表被销毁了。
	WaitThreadRunEnd();
	TransferFds();
	while (m_listFd.size()) {
		int iFd = m_listFd.front();
		m_listFd.pop_front();
		if (iFd >= 0) {
			close(iFd);
		}
	}
}

void SubReactor::AddFd(int iFd) {
	m_iConnectNum++;
	SpinLock::ScopedLock oScopedLock(m_oFdBufferMutex);
	m_listFdBuffer.push_back(iFd);
	write(m_iPipeFds[0], "W", 1);
}

int SubReactor::GetConnectNum() {
	return m_iConnectNum;
}

void SubReactor::InitCoroutines() {
	INFO("begin InitCoroutines ... ");
	// 把日志推到全局缓冲区。
	InitLogReporterCoroutine();
	// 专门将 fd 从 buffer 转移到 fdlist，并唤醒工作协程
	m_vecCoroutine.push_back(new Coroutine(std::bind(&SubReactor::TransferFdsAndWakeUpWorkerCoroutine, this)));
	// 工作协程
	for (int i = 0; i < m_iWorkerCoroutineNum; i++) {
		m_vecCoroutine.push_back(new Coroutine(std::bind(&SubReactor::WorkerCoroutine, this)));
	}
	AllCoroutineStart();
}

void SubReactor::WorkerCoroutine() {
	// DEBUG("one WorkerCoroutine be init");
	const int iBufferSize = 1024;
	char *pBuffer = (char*)malloc(iBufferSize);
	memset(pBuffer, 0, iBufferSize);
	while (true) {
		m_oCoSemaphore.Wait();
		int iFd = m_listFd.front();
		m_listFd.pop_front();
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
		m_iConnectNum--;
	}
	if (pBuffer != nullptr) {
		std::cout << "这里来两次？？？" << std::endl;
		free(pBuffer);
		pBuffer = nullptr;
	}
}

void SubReactor::TransferFdsAndWakeUpWorkerCoroutine() {
	char sBuffer[16];
	while (true) {
		WaitFdEventWithTimeout(m_iPipeFds[1], EPOLLIN, 1000);
		while (read(m_iPipeFds[1], sBuffer, sizeof(sBuffer)) > 0);
		int iTransferNum = TransferFds();
		for (int i = 0; i < iTransferNum; i++) {
			m_oCoSemaphore.Post();
		}
	}
}

void SubReactor::RegisterService(uint32_t iServiceId, std::function<void(const std::string &, std::string &)> funService) {
	g_mapId2Service[iServiceId] = std::move(funService);
}

int SubReactor::TransferFds() {
	SpinLock::ScopedLock oScopedLock(m_oFdBufferMutex);
	int iBufferSize = m_listFdBuffer.size();
	m_listFd.splice(m_listFd.end(), m_listFdBuffer);
	return iBufferSize;
}

}
