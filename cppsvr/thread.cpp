#include "cppsvr/thread.h"
#include "cppsvr/logger.h"
#include "cppsvr/commfunctions.h"

namespace cppsvr {

// 这个在对应 Thread 对象被销毁后就是空指针
static thread_local Thread* t_pThread = nullptr;
// Thread 对象销毁后名字信息还在，只要线程还没被销毁。
static thread_local std::string t_sThreadName = "UNKNOW";

// 创建线程，设置回调函数
Thread::Thread(std::function<void()> funUserFunc, const std::string &sName/* = "UNKNOW"*/)
		: m_funUserFunc(funUserFunc), m_sName(sName) {
	if (sName.empty()) {
		m_sName = "UNKNOW";
	}
	int iRet = pthread_create(&m_tThread, nullptr, &Thread::Run, this);
	if (iRet) {
		ERROR("pthread_create failed. ret %d, name %s", iRet, sName.c_str());
		throw std::logic_error("pthread_create error");
	}
	// 构造未完成的时候等待...
	m_oSemaphore.Wait();
	DEBUG("thread %s be constructed succ.", m_sName.c_str());
}

// 此操作将会把 Thread 销毁，而 pthread_detach 之后的线程会继续执行到结束。
// 也因此要拿信号量控制到至少创建的线程已经拿到回调函数了才到这里，不然那个
// funUserFunc 会被销毁。
Thread::~Thread() {
	if (m_tThread) {
		pthread_detach(m_tThread);
	}
	t_pThread = nullptr;
	DEBUG("thread %s be destroyed.", m_sName.c_str());
}

// 获取当前线程
Thread *Thread::GetThis() {
	return t_pThread;
}

// 获取当前线程名字
const std::string &Thread::GetThreadName() {
	return t_sThreadName;
}

void Thread::SetThreadName(std::string sThreadName) {
	if (t_pThread) {
		t_pThread->m_sName = sThreadName;
	}
	t_sThreadName = sThreadName;
}

// 连接线程，用于阻塞
void Thread::Join() {
	if (m_tThread) {
		int iRet = pthread_join(m_tThread, nullptr);
		if (iRet) {
			ERROR("pthread_join failed. ret %d, name %s", iRet, m_sName.c_str());
			throw std::logic_error("pthread_join error");
		}
		m_tThread = 0;
	}
}

// 线程执行函数
void *Thread::Run(void *arg) {
	Thread *pThread = (Thread *)arg;
	t_pThread = pThread;
	t_sThreadName = pThread->m_sName;
	pThread->m_iId = GetThreadId();

	// Set thread name visible in the kernel and its interfaces.
	pthread_setname_np(pthread_self(), pThread->m_sName.substr(0, 15).c_str());

	std::function<void()> funUserFunc;
	funUserFunc.swap(pThread->m_funUserFunc);
	
	// 拿到回调函数后再通知 Thread 对象可释放
	pThread->m_oSemaphore.Notify();
	funUserFunc();
	return 0;
}

}