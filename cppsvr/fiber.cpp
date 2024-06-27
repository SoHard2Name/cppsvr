#include "cppsvr/fiber.h"
#include "cppsvr/cppsvrconfig.h"
#include <atomic>

namespace cppsvr {

// 这两个就要搞成宏的，函数的话日志参考价值不大
#define GET_CONTEXT(CtxName) \
	if (getcontext(&(CtxName))) { \
		ERROR("get context failed."); \
	}
#define SWAP_CONTEXT(CtxA, CtxB) \
	if (swapcontext(&(CtxA), &(CtxB))) { \
		ERROR("swap context failed."); \
	}


// 当前协程ID
static std::atomic<uint64_t> s_iFiberId{0}; // 加了 atomic 修饰的变量读写操作是原子的
// 全局计数
static std::atomic<uint64_t> s_iFiberCount{0};

// 线程局部变量（当前协程）
static thread_local Fiber *t_pCurrentFiber = nullptr;
// 线程局部变量（**主协程**智能指针）
static thread_local Fiber::ptr t_sptThreadMainFiber = nullptr;

// 配合配置系统指定协程的 栈大小
static uint32_t g_iFiberStackSize = CppSvrConfig::GetSingleton()->GetFiberStackSize();

// 统一的方法来处理协程栈空间的分配
class MallocStackAllocator {
public:
	// 分配协程栈
	static void *Alloc(size_t iSize) {
		return malloc(iSize);
	}
	// 销毁协程栈（这里提供size来支持未来可能的需要）
	static void Dealloc(void *pStack, size_t iSize) {
		return free(pStack);
	}
};

// 使用了个别名 如果未来需要替换栈分配的方式，这里可以比较方便
using StackAllocator = MallocStackAllocator;

// 将当前线程的上下文交由主协程
Fiber::Fiber() {
	m_eState = EXEC;
	SetThis(this);

	// 初始化 context，经过这样设置的后面才能使用。
	GET_CONTEXT(m_oCtx);

	++s_iFiberCount;
}

// 创建子协程
Fiber::Fiber(std::function<void()> funCallBack, size_t iStackSize)
	: m_iId(++s_iFiberId), m_funCallBack(funCallBack) {
	// 统计数加一
	++s_iFiberCount;
	// 栈大小，有设置就用设置的，没设置就用配置文件中的
	m_iStackSize = iStackSize ? iStackSize : g_iFiberStackSize;
	// 分配栈内存
	m_pStack = StackAllocator::Alloc(m_iStackSize);
	// 获取当前上下文
	GET_CONTEXT(m_oCtx);
	// 没有关联上下文（如果有关联上下文，自身结束时会到关联上下文执行）
	m_oCtx.uc_link = nullptr;
	// 指定上下文起始指针
	m_oCtx.uc_stack.ss_sp = m_pStack;
	// 指定栈大小
	m_oCtx.uc_stack.ss_size = m_iStackSize;
	// 创建上下文
	makecontext(&m_oCtx, &Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
	// 统计数减一
	--s_iFiberCount;
	if (m_pStack) { // 子协程会有m_stack
		assert(m_eState == TERM || m_eState == INIT);
		// 回收栈
		StackAllocator::Dealloc(m_pStack, m_iStackSize);
		INFO("one son fiber destruct succ.");
	} else { // 主协程不会有m_stack
		assert(!m_funCallBack);
		assert(m_eState == EXEC);

		if (t_pCurrentFiber == this) {
			SetThis(nullptr);
		}
		INFO("one main fiber destruct succ.");
	}
}

// 重置协程函数，并重置状态，可以复用栈
// INIT，TERM
void Fiber::Reset(std::function<void()> funCallBack) {
	assert(m_pStack);
	assert(m_eState == TERM || m_eState == EXCEPT || m_eState == INIT);
	m_funCallBack = funCallBack;

	GET_CONTEXT(m_oCtx);
	m_oCtx.uc_link = nullptr;
	m_oCtx.uc_stack.ss_sp = m_pStack;
	m_oCtx.uc_stack.ss_size = m_iStackSize;
	makecontext(&m_oCtx, &Fiber::MainFunc, 0);

	m_eState = INIT;
}

void Fiber::Call() {
	m_eState = EXEC;
	INFO("Call one fiber. id %lu", GetId());
	SWAP_CONTEXT(t_sptThreadMainFiber->m_oCtx, m_oCtx);
}

// 切换到当前协程执行(一般是由主协程切换到子协程)
void Fiber::SwapIn() {
	SetThis(this);
	// 原本不是运行状态，现在给他弄到运行状态
	assert(m_eState != EXEC);
	m_eState = EXEC;
	SWAP_CONTEXT(t_sptThreadMainFiber->m_oCtx, m_oCtx);
	// 下次再 swapout 的时候就回到这里，也就是主协程里面执行完这个 swapin 函数的下一句。
}

// 切换到后台执行(一般是由子协程切换到主协程)
void Fiber::SwapOut() {
	SetThis(t_sptThreadMainFiber.get());
	SWAP_CONTEXT(m_oCtx, t_sptThreadMainFiber->m_oCtx);
}

uint64_t Fiber::GetId() const {
	return m_iId;
}

// 设置当前协程
void Fiber::SetThis(Fiber *pCurrentFiber) {
	t_pCurrentFiber = pCurrentFiber;
}

// 返回当前协程
Fiber::ptr Fiber::GetThis() {
	if (t_pCurrentFiber) {
		return t_pCurrentFiber->shared_from_this();
	}
	Fiber::ptr sptMainFiber(new Fiber);
	assert(t_pCurrentFiber == sptMainFiber.get());
	t_sptThreadMainFiber = sptMainFiber;
	return t_pCurrentFiber->shared_from_this();
}

// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
	Fiber::ptr sptCurrentFiber = GetThis();
	sptCurrentFiber->m_eState = READY;
	sptCurrentFiber->SwapOut();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
	Fiber::ptr sptCurrentFiber = GetThis();
	sptCurrentFiber->m_eState = HOLD;
	sptCurrentFiber->SwapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers() {
	return s_iFiberCount;
}

void Fiber::MainFunc() {
	Fiber::ptr sptCurrentFiber = GetThis();
	assert(sptCurrentFiber);
	try {
		sptCurrentFiber->m_funCallBack();
		sptCurrentFiber->m_funCallBack = nullptr;
		sptCurrentFiber->m_eState = TERM;
	} catch (std::exception &ex) {
		sptCurrentFiber->m_eState = EXCEPT;
		ERROR("fiber except: %s. id %lu", ex.what(), sptCurrentFiber->GetId());
	}

	auto raw_ptr = sptCurrentFiber.get();
	sptCurrentFiber.reset();
	raw_ptr->SwapOut();
	// 上面一定会交换走，成功的话一定不会到下面这里。
	ERROR("fiber should never come here. fiber id %lu", raw_ptr->GetId());
}

}