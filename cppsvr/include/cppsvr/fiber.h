#pragma once

#include "thread.h"
#include <functional>
#include <memory>
#include <ucontext.h>

// TODO: 思考如何统一线程第一次调用 GetThis 初始化的协程和后面的工作协程为同一个。

namespace cppsvr {

// enable_shared_from_this 主要作用就是在类的成员函数内部能方便地
// 通过 shared_from_this() 获取到本对象的 spt 指针。然后注意创建
// 对象的时候要使用 spt 创建（如果想要用这个功能）。
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
	typedef std::shared_ptr<Fiber> ptr;

	enum State {
		INIT,	// 初始状态：协程刚被创建时的状态
		HOLD,	// 挂起状态(暂停状态)：协程A未执行完被暂停去执行其他协程，需要保存上下文信息
		EXEC,	// 运行状态(执行中状态)：协程正在执行的状态
		TERM,	// 结束状态：协程运行结束
		READY,	// 就绪状态(可执行状态)：协程可以执行
		EXCEPT, // 异常结束状态：协程发生异常结束
	};

private:
	// 将默认构造函数私有，不允许调用默认构造，由此推断出必然有一个自定义的公共有参构造
	Fiber();

public:
	// 自定义公共有参构造,提供回调方法和最大方法调用栈信息层数
	// 其中的 bUseCaller 表示使用的是 call/back 而不是 swapin/swapout
	Fiber(std::function<void()> funCallBack, size_t iStackSize = 0, bool bUseCaller = false);
	~Fiber();
	// 重置协程函数，并重置状态，重置后该内存可以用在新协程的使用上，节约空间。
	// INIT，TERM
	void Reset(std::function<void()> funCallBack);
	// （第一次初始化得到的协程）调用指定协程执行
	// 开始多协程（第一次初始化的协程并不参与）
	void Call();
	// 从当前协程（通常是主要工作流程）切换回第一次初始化得到的协程
	// 回到单协程
	void Back();
	// （从主要工作流程）切换到当前协程执行
	void SwapIn();
	// 切换到后台执行（从当前协程切换到主要工作流程）
	void SwapOut();
	// 获取协程 id
	uint64_t GetId() const;
	// 获取协程状态
	State GetState() const;
	void SetState(State eState);

public:
	// 设置当前协程
	static void SetThis(Fiber *pCurrentFiber);
	// 返回当前协程
	static Fiber::ptr GetThis();
	// 协程切换到后台，并且设置为Ready状态
	static void YieldToReady();
	// 协程切换到后台，并且设置为Hold状态
	static void YieldToHold();
	// 总协程数
	static uint64_t TotalFibers();

	static void MainFunc();
	static void CallerMainFunc();

private:
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
	std::function<void()> m_funCallBack;
};

}
