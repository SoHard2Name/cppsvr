#include "cppsvr/timer.h"
#include "timer.h"
#include "cppsvr/commfunctions.h"
#include "cppsvr/logger.h"
#include "algorithm"

namespace cppsvr {

thread_local Timer *t_pCurrentTimer = nullptr;

TimeEvent::TimeEvent(std::function<void()> funProcess) : m_funProcess(std::move(funProcess)) {}

// TimeEvent::TimeEvent(std::function<void()> funPrepare, std::function<void()> funProcess)
// 		: m_funPrepare(std::move(funPrepare)), m_funProcess(std::move(funProcess)) {}

TimeEvent::TimeEvent(uint64_t iExpireTime, std::function<void()> funPrepare,
					 std::function<void()> funProcess, uint32_t iInterval /* = 0*/)
	: m_iExpireTime(iExpireTime), m_funPrepare(std::move(funPrepare)), m_funProcess(std::move(funProcess)),
	  m_iBelongList(-1), m_itIterInList(nullptr), m_iInterval(iInterval) {}

Timer::Timer() : m_iCurrentTime(0), m_vecTimeEvent(60 * 1000), m_iCurrentIndex(-1) {}

Timer *Timer::GetThis() {
	return t_pCurrentTimer;
}

void Timer::SetThis(Timer *pCurrentTimer) {
	t_pCurrentTimer = pCurrentTimer;
}

void Timer::AddRelativeTimeEvent(uint32_t iRelativeTime, std::function<void()> funPrepare, std::function<void()> funProcess, uint32_t iInterval/* = 0*/) {
	AddTimeEvent(std::make_shared<TimeEvent>(GetCurrentTimeMs() + iRelativeTime, std::move(funPrepare), std::move(funProcess), iInterval));
}

void Timer::AddAbsoluteTimeEvent(uint64_t iAbsoluteTime, std::function<void()> funPrepare, std::function<void()> funProcess, uint32_t iInterval/* = 0*/) {
	AddTimeEvent(std::make_shared<TimeEvent>(iAbsoluteTime, std::move(funPrepare), std::move(funProcess), iInterval));
}

void Timer::AddTimeEvent(TimeEvent::ptr pTimeEvent) {
	if (!m_iCurrentTime) {
		m_iCurrentTime = GetCurrentTimeMs();
		m_iCurrentIndex = 0;
	}
	if (pTimeEvent->m_iExpireTime < m_iCurrentTime) {
		ERROR("event expire time is smaller than now");
		return;
	}
	uint32_t iSize = m_vecTimeEvent.size();
	int iTimeDiff = std::min<uint64_t>(pTimeEvent->m_iExpireTime - m_iCurrentTime, iSize - 1);
	int iBelongList = (m_iCurrentIndex + iTimeDiff) % iSize + 1;
	auto *pList = &m_vecTimeEvent[iBelongList];
	pList->push_back(pTimeEvent);
	pList->back()->m_iBelongList = iBelongList;
	pList->back()->m_itIterInList = --pList->end();
}

void Timer::DeleteTimeEvent(TimeEvent::ptr pTimeEvent) {
	m_vecTimeEvent[pTimeEvent->m_iBelongList].erase(pTimeEvent->m_itIterInList);
}

void Timer::GetAllTimeoutEvent(std::list<TimeEvent::ptr> &listResult) {
	if (!m_iCurrentTime) {
		return;
	}
	uint64_t iPreTime = m_iCurrentTime;
	m_iCurrentTime = GetCurrentTimeMs();
	uint32_t iCount = m_iCurrentTime - iPreTime + 1;
	for (int i = 0; i < iCount; i++) {
		auto &listTimeEvent = m_vecTimeEvent[(m_iCurrentIndex + i) % m_vecTimeEvent.size()];
		for (auto it = listTimeEvent.begin(); it != listTimeEvent.end(); it++) {
			TimeEvent::ptr pTimeEvent = *it;
			if (pTimeEvent->m_iExpireTime > m_iCurrentTime) {
				AddTimeEvent(pTimeEvent);
			} else {
				listResult.push_back(pTimeEvent);
				// 是重复事件，还要再添加上去新事件。注意到这里会改动已经 push 进
				// result 列表的内容，但没事，因为用到时候用的是里面的函数而已。
				if (pTimeEvent->m_iInterval > 0) {
					uint32_t iInterval = pTimeEvent->m_iInterval;
					// 算准下次会出现的地方
					uint64_t iTimeDiff = ((m_iCurrentTime - pTimeEvent->m_iExpireTime) / iInterval + 1) * iInterval;
					pTimeEvent->m_iExpireTime += iTimeDiff;
					// 添加新事件
					AddTimeEvent(pTimeEvent);
				}
			}
		}
		listTimeEvent.clear();
	}
}
}