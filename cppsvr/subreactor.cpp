#include "cppsvr/subreactor.h"
#include "cppsvr/commfunctions.h"
#include "cstring"

namespace cppsvr {

// 初始化。
std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> SubReactor::g_mapId2Service;

SubReactor::SubReactor(uint32_t iWorkerCoroutineNum/* = 配置数*/) : m_iWorkerCoroutineNum(iWorkerCoroutineNum), 
		m_iConnectNum(0), m_listFdBuffer(), m_listFd(), m_oCoSemaphore(0) {

	memset(iPipeFds, -1, sizeof(iPipeFds));
	assert(!pipe(iPipeFds));

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
	WaitThreadRunEnd();
	if (m_iListenFd >= 0) {
		close(m_iListenFd);
	}
	while (m_listFdBuffer.size()) {
		int iFd = m_listFdBuffer.front();
		m_listFdBuffer.pop_front();
		if (iFd >= 0) {
			close(iFd);
		}
	}
	while (m_listFd.size()) {
		int iFd = m_listFd.front();
		m_listFd.pop_front();
		if (iFd >= 0) {
			close(iFd);
		}
	}
}

void SubReactor::InitCoroutines() {
	INFO("begin InitCoroutines ... ");
	// assert(0);
	CoroutinePool::InitCoroutines();
	// // 初始化 accept 协程
	// m_vecCoroutine.push_back(new Coroutine(std::bind(&SubReactor::AcceptCoroutine, this)));
	// 剩下的都是 read write 协程，里面也处理业务
	for (int i = 0; i < m_iWorkerCoroutineNum; i++) {
		m_vecCoroutine.push_back(new Coroutine(std::bind(&SubReactor::ReadWriteCoroutine, this)));
	}
	AllCoroutineStart();
}

void SubReactor::ReadWriteCoroutine() {
	// DEBUG("one ReadWriteCoroutine be init");
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


void SubReactor::RegisterService(uint32_t iServiceId, std::function<void(const std::string &, std::string &)> funService) {
	g_mapId2Service[iServiceId] = std::move(funService);
}

}
