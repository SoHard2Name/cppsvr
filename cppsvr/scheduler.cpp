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

		assert(GetThis() == nullptr); // 本线程已经有人在调度我了，我不能再调度我自己
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
		assert(GetThis() == this); // 是个任务线程，并且这个任务线程的调度器就是它自己
	} else {
		assert(GetThis() != this); // 不是饿汉调度器，自己不会调度到自己但是并不能直接定为空指针，特殊情况
								   // 是比如调度器的一个任务里面又创建了一个调度器，那这个线程其实也是被调
								   // 度的，只不过不是自身。
	}

	m_stopping = true;
	for (size_t i = 0; i < m_threadCount; ++i) {
		tickle();
	}
	if (m_rootFiber) {
		tickle();
	}
	
	// 这个参与分担任务的行为我是认可的，因为不然这个线程其实就是在等别人做完而已，自己
	// 也没什么事情干，这样弄的意义相当于不让它白占内存。
	// 但是整个流程实现的太屎了，后面再给他封装的合理一些。。。
	// TODO:
	// m_autoStop 这个变量就是完全没必要的。
	// 上面那个 return 的特殊情况是不是有点多余？
	// 这个 idle 方法应该是使线程被阻塞才对而不是死循环，应该搞个睡眠-唤醒机制来调控或者是定时器？
	// 分 start 和 stop 方法是不是没必要的？直接把 start 弄到构造函数，stop 就是 start
	// 当然现在只是一开始，后面会慢慢改进。
	if (m_rootFiber && !stopping()) {
		// 注意这个 m_rootFiber 才是工作的主流程，而不是第一次调用 GetThis 初始化的协程
		m_rootFiber->Call();
		INFO("main work fiber call end. state %d", m_rootFiber->GetState());
	}
	
	// 等待每个子线程都释放了
	std::vector<Thread::ptr> thrs;
    {
        MutexType::ScopedLock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto& i : thrs) {
        i->Join();
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
        bool is_active = false;
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
                ++m_activeThreadCount;
				is_active = true;
				break;
			}
		}

		if (tickle_me) {
			tickle();
		}

		// 协程式任务
		if (ft.fiber && (ft.fiber->GetState() != Fiber::TERM && ft.fiber->GetState() != Fiber::EXCEPT)) {
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
			if (is_active) {
				--m_activeThreadCount;
				continue;
			}
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
	while (!stopping()) {
		Fiber::GetThis()->YieldToHold();
	}
}

}
