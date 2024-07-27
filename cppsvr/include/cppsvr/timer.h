#pragma once
#include "list"
#include "vector"
#include "cstdint"
#include "functional"
#include "memory"

namespace cppsvr {

struct TimeEvent {
	typedef std::shared_ptr<TimeEvent> ptr;
	TimeEvent(std::function<void()> funProcess);
	TimeEvent(uint64_t iExpireTime, std::function<void()> funPrepare, std::function<void()> funProcess, uint32_t iInterval = 0);
	uint64_t m_iExpireTime = UINT64_MAX; // 期待触发的准确时间戳
	std::function<void()> m_funPrepare = nullptr; // 就绪才会触发（主要就是提前取消超时事件）
	std::function<void()> m_funProcess = nullptr; // 执行函数（不管有没有就绪）
	uint32_t m_iInterval = 0; // 重复的时间间隔，为 0 表示不重复
	int m_iBelongList = 0; // 事件所处的列表（的下标）
	std::list<TimeEvent::ptr>::iterator m_itIterInList = {}; // 事件的迭代器
};


class Timer {
public:
	Timer();

	static Timer *GetThis();
	static void SetThis(Timer *pCurrentTimer);

	// 特别注意：传的是相对时间！一般只会用到这个
	TimeEvent::ptr AddRelativeTimeEvent(uint32_t iRelativeTime, std::function<void()> funPrepare, std::function<void()> funProcess, uint32_t iInterval = 0);
	TimeEvent::ptr AddAbsoluteTimeEvent(uint64_t iAbsoluteTime, std::function<void()> funPrepare, std::function<void()> funProcess, uint32_t iInterval = 0);
	// 注意这些直接用到 time event 结构体的，传的是绝对时间
	TimeEvent::ptr AddTimeEvent(TimeEvent::ptr pTimeEvent);
	void DeleteTimeEvent(TimeEvent::ptr pTimeEvent);
	void GetAllTimeoutEvent(std::list<TimeEvent::ptr> &listResult);

private:
	uint64_t m_iCurrentTime; // 当前时间戳
	std::vector<std::list<TimeEvent::ptr>> m_vecTimeEvent; // 轮盘
	int m_iCurrentIndex; // 当前指向哪个事件列表
};

}