#pragma once
#include "memory"
#include "functional"
#include "ucontext.h"
#include "mutex.h"

// 协程这种东西肯定是不能同时有两个线程来执行它的，虽然它可以来回切换，
// 但任何时刻只允许一个线程在执行它，所以弄一个自旋锁，当进入协程的时候
// 锁住，出来的时候解锁。

// TODO：把协程实现为汇编语言那种。

namespace cppsvr {

// 这些就要搞成宏的，函数的话日志参考价值不大
#define GET_CONTEXT(CtxName) \
	if (getcontext(&(CtxName))) { \
		ERROR("get context failed."); \
	}
#define SWAP_CONTEXT(CtxA, CtxB) \
	if (swapcontext(&(CtxA), &(CtxB))) { \
		ERROR("swap context failed."); \
	}
#define SET_CONTEXT(Ctx) \
	if (setcontext(&(Ctx))) { \
		ERROR("set context failed."); \
	}

class Coroutine : public std::enable_shared_from_this<Coroutine> {
public:
	typedef std::shared_ptr<Coroutine> ptr;

	enum State {
		INIT,	// 初始状态：协程刚被创建时的状态
		HOLD,	// 挂起状态(暂停状态)：协程A未执行完被暂停去执行其他协程，需要保存上下文信息
		EXEC,	// 运行状态(执行中状态)：协程正在执行的状态
		TERM,	// 结束状态：协程运行结束
		EXCEPT, // 异常结束状态：协程发生异常结束
	};

private:
	Coroutine(int); // 用于创建主协程。
	
public:
	Coroutine(std::function<void()> funUserFunc = nullptr, size_t iStackSize = 0);
	~Coroutine();
	void SetUserFunc(std::function<void()> funUserFunc);
	void SwapIn();
	void SwapOut(State eState = HOLD);
	uint64_t GetId() const;
	State GetState() const;
	void SetState(State eState);

public:
	static ptr GetThis();
	static uint64_t TotalCoroutines();

	static void MainFunc();

private:
	static void SetThis(Coroutine *pCurrentCoroutine);
	// 协程ID
	uint64_t m_iId = 0;
	// 协程栈大小
	uint32_t m_iStackSize = 0;
	// 协程状态
	State m_eState = INIT;
	// 上下文
	ucontext_t m_oCtx;
	// 当前栈指针
	void *m_pStack = nullptr;
	// 回调函数(协程中运行的方法)
	std::function<void()> m_funUserFunc;
	// 应该返回的协程指针（是它来调我的）
	ptr m_pReturnCoroutine;
};

}
