

#include "utils/fiber.h"
#include "scheduler.h"
#include "utils/configbase.h"
#include <atomic>

namespace utility {

// 当前协程ID
static std::atomic<uint64_t> s_fiber_id{0};
// 全局计数
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量（当前协程）
static thread_local Fiber *t_fiber = nullptr;
// 线程局部变量（主协程智能指针）
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;

// 配合配置系统指定协程的 栈大小
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
	Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

// 统一的方法来处理协程栈空间的分配
class MallocStackAllocator {
public:
	// 分配协程栈
	static void *Alloc(size_t size) {
		return malloc(size);
	}
	// 销毁协程栈（这里提供size来支持未来可能的需要）
	static void Dealloc(void *vp, size_t size) {
		return free(vp);
	}
};

// 使用了个别名 如果未来需要替换栈分配的方式，这里可以比较方便
using StackAllocator = MallocStackAllocator;

// 将当前线程的上下文交由主协程
Fiber::Fiber() {
	m_state = EXEC;
	SetThis(this);

	if (getcontext(&m_ctx)) {
		SYLAR_ASSERT(false, "getcontext");
	}

	++s_fiber_count;
}

// 创建子协程
Fiber::Fiber(std::function<void()> cb, size_t stacksize)
	: m_id(++s_fiber_id), m_cb(cb) {
	// 统计数加一
	++s_fiber_count;
	// 栈大小，有设置就用设置的，没设置就用配置文件中的
	m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
	// 分配栈内存
	m_stack = StackAllocator::Alloc(m_stacksize);
	// 获取当前上下文
	if (getcontext(&m_ctx)) {
		SYLAR_ASSERT(false, "getcontext");
	}
	// 没有关联上下文（如果有关联上下文，自身结束时会回到关联上下文执行）
	m_ctx.uc_link = nullptr;
	// 指定上下文起始指针
	m_ctx.uc_stack.ss_sp = m_stack;
	// 指定栈大小
	m_ctx.uc_stack.ss_size = m_stacksize;
	// 创建上下文
	makecontext(&m_ctx, &Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
	// 统计数减一
	--s_fiber_count;
	if (m_stack) { // 子协程会有m_stack
		SYLAR_ASSERT(m_state == TERM || m_state == INIT);
		// 回收栈
		StackAllocator::Dealloc(m_stack, m_stacksize);
	} else { // 主协程不会有m_stack
		SYLAR_ASSERT(!m_cb);
		SYLAR_ASSERT(m_state == EXEC);

		Fiber *cur = t_fiber;
		if (cur == this) {
			SetThis(nullptr);
		}
	}
}

// 重置协程函数，并重置状态,可以复用栈
// INIT，TERM
void Fiber::reset(std::function<void()> cb) {
	SYLAR_ASSERT(m_stack);
	SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
	m_cb = cb;
	if (getcontext(&m_ctx)) {
		SYLAR_ASSERT2(false, "getcontext");
	}

	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

	makecontext(&m_ctx, &Fiber::MainFunc, 0);
	m_state = INIT;
}

void Fiber::call() {
	m_state = EXEC;
	SYLAR_LOG_ERROR(g_logger) << getId();
	if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
		SYLAR_ASSERT2(false, "swapcontext");
	}
}

// 切换到当前协程执行(一般是由主协程切换到子协程)
void Fiber::swapIn() {
	SetThis(this);
	SYLAR_ASSERT(m_state != EXEC);
	m_state = EXEC;
	if (swapcontext(&(*t_threadFiber)->m_ctx, &m_ctx)) {
		SYLAR_ASSERT2(false, "swapcontext");
	}
}

// 切换到后台执行(一般是由子协程切换到主协程)
void Fiber::swapOut() {
	SetThis((*t_threadFiber).get());

	if (swapcontext(&m_ctx, &(*t_threadFiber)->m_ctx)) {
		SYLAR_ASSERT2(false, "swapcontext");
	}
}

// 设置当前协程
void Fiber::SetThis(Fiber *f) {
	t_fiber = f;
}

// 返回当前协程
Fiber::ptr Fiber::GetThis() {
	if (t_fiber) {
		return t_fiber->shared_from_this();
	}
	Fiber::ptr main_fiber(new Fiber);
	SYLAR_ASSERT(t_fiber == main_fiber.get());
	t_threadFiber = main_fiber;
	return t_fiber->shared_from_this();
}

// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
	Fiber::ptr cur = GetThis();
	cur->m_state = READY;
	cur->swapOut();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
	Fiber::ptr cur = GetThis();
	cur->m_state = HOLD;
	cur->swapOut();
}

// 总协程数
uint64_t Fiber::TotalFibers() {
	return s_fiber_count;
}

void Fiber::MainFunc() {
	Fiber::ptr cur = GetThis();
	SYLAR_ASSERT(cur);
	try {
		cur->m_cb();
		cur->m_cb = nullptr;
		cur->m_state = TERM;
	} catch (std::exception &ex) {
		cur->m_state = EXCEPT;
		SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
								  << " fiber_id=" << cur->getId()
								  << std::endl
								  << sylar::BacktraceToString();
	} catch (...) {
		cur->m_state = EXCEPT;
		SYLAR_LOG_ERROR(g_logger) << "Fiber Except"
								  << " fiber_id=" << cur->getId()
								  << std::endl
								  << sylar::BacktraceToString();
	}

	auto raw_ptr = cur.get();
	cur.reset();
	raw_ptr->swapOut();

	SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}