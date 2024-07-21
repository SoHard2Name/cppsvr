#include "cppsvr/comutex.h"
#include "cppsvr/coroutinepool.h"

namespace cppsvr {

CoSemaphore::CoSemaphore(int iCount/* = 0*/) : m_iCount(iCount), m_listWait() {}

void CoSemaphore::Post() {
	if (++m_iCount <= 0) {
		auto pTimeEvent = m_listWait.front();
		m_listWait.pop_front();
		CoroutinePool::GetThis()->AddActive(pTimeEvent);
	}
}

void CoSemaphore::Wait() {
	if (m_iCount-- <= 0) {
		auto pTimeEvent = std::make_shared<TimeEvent>(std::bind(&CoroutinePool::DefaultProcess, Coroutine::GetThis()));
		m_listWait.push_back(pTimeEvent);
		Coroutine::GetThis()->SwapOut();
	}
}

}