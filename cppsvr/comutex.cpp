#include "cppsvr/comutex.h"
#include "cppsvr/coroutinepool.h"

namespace cppsvr {

CoSemaphore::CoSemaphore(int iCount/* = 0*/) : m_iCount(iCount), m_listWait() {
	WARN("construct one CoSemaphore, count %d", m_iCount);
}

void CoSemaphore::Post() {
	if (++m_iCount <= 0) {
		// INFO("m_iCount %d, m_listWait size %zu", m_iCount, m_listWait.size());
		// WARN("front use count %ld", m_listWait.front().use_count());
		// WARN("can come here..0");
		TimeEvent::ptr pTimeEvent = m_listWait.front();
		// WARN("can come here..1");
		m_listWait.pop_front();
		// WARN("can come here..2");
		CoroutinePool::GetThis()->AddActive(pTimeEvent);
		// WARN("cannot come here!!!");
	}
}

void CoSemaphore::Wait() {
	if (m_iCount-- <= 0) {
		auto pTimeEvent = std::make_shared<TimeEvent>(std::bind(&CoroutinePool::DefaultProcess, Coroutine::GetThis()));
		m_listWait.push_back(pTimeEvent);
		Coroutine::GetThis()->SwapOut();
	}
}

int CoSemaphore::GetCount() {
	return m_iCount;
}

}