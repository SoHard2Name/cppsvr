#pragma once
#include "memory"
#include "functional"

namespace cppsvr {

class Coroutine {
public:
	Coroutine(std::function<void()> funUserFunc, int iStackSize);

	struct CoroutineContext {
		CoroutineContext(int iStackSize);
		~CoroutineContext();
		void *m_pArgs[8];
		char *m_pStack;
		int m_iStackSize;
	};

	static Coroutine *GetThis();
	static void SetThis(Coroutine* pCoroutine);
	
	void SwapIn();
	void SwapOut();
	
private:
	Coroutine();
	static void DoWork(Coroutine *pCoroutine);

private:
	int m_iId;
	CoroutineContext m_oContext;
	Coroutine *m_pFather;
	std::function<void()> m_funUserFunc;
};

}
