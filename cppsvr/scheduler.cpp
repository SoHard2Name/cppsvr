#include "cppsvr/scheduler.h"
#include "cppsvr/logger.h"
#include "cppsvr/commfunctions.h"

namespace cppsvr {

static thread_local Scheduler *t_scheduler = nullptr;
// 主要工作流程的协程
static thread_local Fiber *t_fiber = nullptr;

Scheduler::Scheduler(size_t threads/* = 1*/, bool use_caller/* = true*/, const std::string &name/* = ""*/)
	: m_name(name) {
	assert(threads > 0);
	// 本调度线程也作为工作线程。
	if (use_caller) {
		cppsvr::Fiber::GetThis();
		--threads;

		assert(GetThis() == nullptr);
		t_scheduler = this;

		m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // 这个
		cppsvr::Thread::SetThreadName(m_name);

		t_fiber = m_rootFiber.get();
		m_rootThread = cppsvr::GetThreadId();
		m_threadIds.push_back(m_rootThread);
	} else {
		m_rootThread = -1;
	}
	m_threadCount = threads;
}

Scheduler::~Scheduler() {
	assert(m_stopping);
	if (GetThis() == this) {
		t_scheduler = nullptr;
	}
}

Scheduler *Scheduler::GetThis() {
	return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() {
	return t_fiber;
}

void Scheduler::start() {
	MutexType::ScopedLock lock(m_mutex);
	if (!m_stopping) {
		return;
	}
	m_stopping = false;
	assert(m_threads.empty());

	m_threads.resize(m_threadCount);
	for (size_t i = 0; i < m_threadCount; ++i) {
		m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
		m_threadIds.push_back(m_threads[i]->GetId());
	}
	lock.Unlock();

	// 为什么非要搞出一个 m_rootFiber 协程来当主要工作流程？
	// 其他它本身主流程也是可以通过直接调用 run 来当做工作流程的。
	// 但是可能考虑到后续如只阻塞这个协程，不会影响到主线程的进行
	// 而如果直接拿主协程来阻塞，相当于把主线程直接阻塞了，这里先
	// 这样实现。
	if (m_rootFiber) {
		// 注意这个 m_rootFiber 才是工作的主流程，而不是第一次调用 GetThis 初始化的协程
		m_rootFiber->Call();
		INFO("main work fiber call end. state %d", m_rootFiber->GetState());
	}
}

void Scheduler::stop() {
	m_autoStop = true;
	if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->GetState() == Fiber::TERM || m_rootFiber->GetState() == Fiber::INIT)) {
		INFO("scheduler stop");
		m_stopping = true;

		if (stopping()) {
			return;
		}
	}

	// bool exit_on_this_fiber = false;
	if (m_rootThread != -1) {
		assert(GetThis() == this);
	} else {
		assert(GetThis() != this);
	}

	m_stopping = true;
	for (size_t i = 0; i < m_threadCount; ++i) {
		tickle();
	}

	if (m_rootFiber) {
		tickle();
	}

	if (stopping()) {
		return;
	}

	// if(exit_on_this_fiber) {
	// }
}

// 设置当前线程的调度器
void Scheduler::setThis() {
	t_scheduler = this;
}

void Scheduler::run() {
	INFO("run");
	setThis();

	// 如果是工作线程，则初始化得到的协程也就是其主要工作流程。
	if (cppsvr::GetThreadId() != m_rootThread) {
		t_fiber = Fiber::GetThis().get();
	}

	Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
	Fiber::ptr cb_fiber;

	FiberAndThread ft;
	while (true) {
		ft.reset();
		bool tickle_me = false;
		{
			MutexType::ScopedLock lock(m_mutex);
			auto it = m_fibers.begin();
			while (it != m_fibers.end()) {
				if (it->thread != -1 && it->thread != cppsvr::GetThreadId()) {
					++it;
					tickle_me = true;
					continue;
				}

				assert(it->fiber || it->cb);
				if (it->fiber && it->fiber->GetState() == Fiber::EXEC) {
					++it;
					continue;
				}

				ft = *it;
				m_fibers.erase(it);
				break;
			}
		}

		if (tickle_me) {
			tickle();
		}

		// 协程式任务
		if (ft.fiber && (ft.fiber->GetState() != Fiber::TERM && ft.fiber->GetState() != Fiber::EXCEPT)) {
			++m_activeThreadCount;
			ft.fiber->SwapIn();
			--m_activeThreadCount;

			if (ft.fiber->GetState() == Fiber::READY) {
				schedule(ft.fiber);
			} else if (ft.fiber->GetState() != Fiber::TERM && ft.fiber->GetState() != Fiber::EXCEPT) {
				ft.fiber->SetState(Fiber::HOLD);
			}
			ft.reset();
		} else if (ft.cb) { // 函数式任务
			if (cb_fiber) { // 有协程了就复用
				cb_fiber->Reset(ft.cb);
			} else { // 没有就创建一个
				cb_fiber.reset(new Fiber(ft.cb));
			}
			ft.reset();
			++m_activeThreadCount;
			cb_fiber->SwapIn();
			--m_activeThreadCount;
			if (cb_fiber->GetState() == Fiber::READY) {
				schedule(cb_fiber);
				cb_fiber.reset();
			} else if (cb_fiber->GetState() == Fiber::EXCEPT || cb_fiber->GetState() == Fiber::TERM) {
				cb_fiber->Reset(nullptr);
			} else {
				cb_fiber->SetState(Fiber::HOLD);
				cb_fiber.reset();
			}
		} else { // 无任务
			if (idle_fiber->GetState() == Fiber::TERM) {
				INFO("idle fiber term");
				break;
			}

			++m_idleThreadCount;
			idle_fiber->SwapIn();
			--m_idleThreadCount;
			if (idle_fiber->GetState() != Fiber::TERM && idle_fiber->GetState() != Fiber::EXCEPT) {
				idle_fiber->SetState(Fiber::HOLD);
			}
		}
	}
}

void Scheduler::tickle() {
	INFO("tickle");
}

bool Scheduler::stopping() {
	MutexType::ScopedLock lock(m_mutex);
	return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
	INFO("idle");
}

}
