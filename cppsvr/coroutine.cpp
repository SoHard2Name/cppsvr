#include "cppsvr/coroutine.h"
#include "atomic"
#include "cassert"
#include "cstring"

namespace cppsvr {

// 用于计算协程 id
static std::atomic<uint64_t> g_iCoroutineCount{0};

// 线程的当前协程
static thread_local Coroutine *t_pCurrentCoroutine = nullptr;
// 线程的主协程
static thread_local Coroutine *t_pMainCoroutine = nullptr;

Coroutine::CoroutineContext::CoroutineContext(uint32_t iStackSize, Coroutine *pCoroutine) :
		m_pStack(nullptr), m_iStackSize(iStackSize) {
	// 由于 m_pArgs 是数组，所以就直接 sizeof 它了
	memset(m_pArgs, 0, sizeof(m_pArgs));
	if (!iStackSize) {
		return;
	}
	m_pStack = (char*)malloc(iStackSize);
	
	// 注意栈是从高地址向低地址延伸的。
	// 留一个指针的位置，写入协程对应的函数地址（即 DoWork）。
	char *rsp = m_pStack + iStackSize - sizeof(void*);
	// 保证整除十六字节，这个是操作系统要求的
	rsp = (char*)((uint64_t)rsp & -16LL);
	
	m_pArgs[1] = rsp;
	*((void**)rsp) = (void*)&DoWork;
	
	m_pArgs[7] = pCoroutine;
}

Coroutine::CoroutineContext::~CoroutineContext() {
	if (m_pStack) {
		free(m_pStack);
		m_pStack = nullptr;
	}
}

Coroutine::Coroutine(char) : m_iId(0), m_oContext(0, 0), 
	m_pFather(nullptr), m_funUserFunc(nullptr) {}

Coroutine::Coroutine(std::function<void()> funUserFunc, uint32_t iStackSize/* = 配置的大小*/) 
	: m_iId(++g_iCoroutineCount), m_oContext(iStackSize, this), 
	  m_pFather(nullptr), m_funUserFunc(std::move(funUserFunc)) {}

Coroutine *Coroutine::GetThis() {
	if (t_pCurrentCoroutine == nullptr) {
		assert(!t_pMainCoroutine);
		t_pCurrentCoroutine = t_pMainCoroutine = new Coroutine(' ');
	}
	return t_pCurrentCoroutine;
}

void Coroutine::SetThis(Coroutine *pCoroutine) {
	t_pCurrentCoroutine = pCoroutine;
}

void Coroutine::DoWork(Coroutine *pCoroutine) {
	if (pCoroutine->m_funUserFunc) {
		pCoroutine->m_funUserFunc();
		pCoroutine->m_funUserFunc = nullptr;
	}
	pCoroutine->SwapOut();
	ERROR("coroutine can not come here.");
	assert(0);
}

// 用于切换协程的函数，指定其汇编语言为 asm 符号 CoSwap。
extern void CoSwap(Coroutine::CoroutineContext* pCurrentCoroutine, 
	Coroutine::CoroutineContext* pFutureCoroutine) asm("CoSwap");

uint64_t Coroutine::GetId() {
	return m_iId;
}

void Coroutine::SwapIn() {
	assert (this != GetThis());
	m_pFather = GetThis();
	SetThis(this);
	CoSwap(&m_pFather->m_oContext, &m_oContext);
}

void Coroutine::SwapOut() {
	assert(m_pFather);
	SetThis(m_pFather);
	CoSwap(&m_oContext, &m_pFather->m_oContext);
}

}
