#pragma once
#include "timer.h"
#include <list>

namespace cppsvr {

// 只针对同一线程的协程间竞争，如果不同线程的竞争那就要用
// 内核提供的互斥量了，自然也不用我这里提供。
// 互斥锁什么的是完全不用的，主要就是一个信号量，同步一下
// 生产者和消费者。

class CoSemaphore {
public:
	CoSemaphore(int iCount = 0);
	void Post();
	void Wait();
private:
	int m_iCount;
	std::list<TimeEvent::ptr> m_listWait;
};

}
