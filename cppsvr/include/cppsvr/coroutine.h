#pragma once
#include "memory"
#include "functional"
#include "cppsvrconfig.h"

namespace cppsvr {

class Coroutine {
public:
	Coroutine(std::function<void()> funUserFunc, uint32_t iStackSize = CppSvrConfig::GetSingleton()->GetCoroutineStackSize());

	struct CoroutineContext {
		CoroutineContext(uint32_t iStackSize, Coroutine *pCoroutine);
		~CoroutineContext();
		void *m_pArgs[8];
		char *m_pStack;
		uint32_t m_iStackSize;
	};

	static Coroutine *GetThis();
	static void SetThis(Coroutine* pCoroutine);
	
	uint64_t GetId();
	void SwapIn();
	void SwapOut();
	
private:
	Coroutine(char);
	static void DoWork(Coroutine *pCoroutine);

private:
	uint64_t m_iId;
	CoroutineContext m_oContext;
	Coroutine *m_pFather;
	std::function<void()> m_funUserFunc;
};

}
