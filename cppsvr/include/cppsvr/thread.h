#pragma once

#include "memory"
#include "functional"
#include "pthread.h"
#include "iostream"
#include "mutex.h"
#include "noncopyable.h"
#include "coroutine.h"
#include "vector"

namespace cppsvr {

// Thread 对象属于创建者线程的，里面维护了一些被创建线程的信息
class Thread {
	NON_COPY_ABLE(Thread);
public:
	typedef std::shared_ptr<Thread> ptr;
	Thread(std::function<void()> funUserFunc, const std::string &sName = "UNKNOW");
	~Thread();

	pid_t GetId() const { return m_iId; }
	const std::string &GetName() const { return m_sName; }
	void Join();

	// 获取 Thread 对象的指针
	static Thread *GetThis();
	static const std::string &GetThreadName();
	static void SetThreadName(std::string sThreadName);

private:
	static void *Run(void *arg);

private:
	pid_t m_iId = -1;
	pthread_t m_tThread = 0; // 这个没办法，这样命名好理解一点。
	std::function<void()> m_funUserFunc;
	std::string m_sName;
	// 用于保证线程拿到回调函数之后这个 Thread 对象才被销毁
	Semaphore m_oSemaphore;
};

}