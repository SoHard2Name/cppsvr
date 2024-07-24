#pragma once
#include "coroutinepool.h"
#include "logger.h"

namespace cppsvr {

// 主响应器只有一个，负责 accept，并发送给还未处理完的连接最多的子响应器。
// 以及负责把全局缓存的日志内容存到磁盘中。

class MainReactor : public CoroutinePool {
public:
	MainReactor(uint32_t iCoroutineNum = CppSvrConfig::GetSingleton()->GetCoroutineNum());
	~MainReactor();

	virtual void InitCoroutines() override;
	void AcceptCoroutine();

private:
	int m_iListenFd;
};


}