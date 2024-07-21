#pragma once
#include "coroutinepool.h"
#include "cppsvrconfig.h"
#include "comutex.h"
#include "queue"
#include "unordered_map"
#include "string"

namespace cppsvr {

class ServerCoroutinePool : public CoroutinePool {
public:
	ServerCoroutinePool(uint32_t iCoroutineNum = CppSvrConfig::GetSingleton()->GetCoroutineNum());
	void InitCoroutines();

	void AcceptCoroutine();
	void ReadWriteCoroutine();
	
	// 如果失败或者超时了都自动关闭 fd。
	static int Read(int iFd, std::string &sMessage, uint32_t iRelativeTimeout = UINT32_MAX);
	static int Write(int iFd, std::string &sMessage, uint32_t iRelativeTimeout = UINT32_MAX);
	
public:
	// 每个函数都是一进一出，进表示 req，出表示 resp。
	// 每个服务器只需要通过在这个 map 上面注册服务就行，就是实现的时候还是要会使用本框架实现的 Read 函数之类。
	static std::unordered_map<uint32_t, std::function<void(const std::string&, std::string&)>> g_mapId2Service;
	
private:
	int m_iListenFd;
	std::queue<int> m_queFd;
	CoSemaphore m_oCoSemaphore;
};

}
