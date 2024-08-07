#pragma once
#include "coroutinepool.h"
#include "subreactor.h"
#include "logger.h"
#include "cstring"

namespace cppsvr {

// 主响应器只有一个，负责 accept，并发送给还未处理完的连接最多的子响应器。
// 以及负责把全局缓存的日志内容存到磁盘中。

class MainReactor : public CoroutinePool {
public:
	MainReactor(uint32_t iWorkerThreadNum = cppsvr::CppSvrConfig::GetSingleton()->GetWorkerThreadNum(), 
				uint32_t iWorkerCoroutineNum = cppsvr::CppSvrConfig::GetSingleton()->GetWorkerCoroutineNum());
	~MainReactor();

	virtual void Run(bool bUseCaller = false) override;
	virtual void InitCoroutines() override;

private:
	void AcceptCoroutine();

private:
	int m_iListenFd;
	std::vector<SubReactor*> m_vecSubReactor;
};


}