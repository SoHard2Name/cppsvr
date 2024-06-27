#include "cppsvr/cppsvr.h"
#include "functional"
#include "iostream"
#include "list"


cppsvr::Mutex oLoggerMutex;

// 这个只是临时用的，因为还是日志器还是同步的嘛。
#define TEMP_ASYN_LOG(Format, ...)                 \
	{                                              \
		cppsvr::Mutex::ScopedLock X(oLoggerMutex); \
		INFO(Format, ##__VA_ARGS__);               \
	}

class Scheduler {
public:
	Scheduler(uint32_t iThreadNum) : m_iThreadNum(iThreadNum), 
		m_vecThreadPool(iThreadNum) {} // 记得初始化线程池。。

	// 目前保证的是这个只有单线程的情况下会用，所以暂时不用加锁。
	void AddTask(std::function<void()> funTask) {
		m_listTasks.push_back(funTask);
	}

	void Run() {
		for (int i = 0; i < m_iThreadNum; i++) {
			m_vecThreadPool[i] = std::make_shared<cppsvr::Thread>(
									// 成员函数要求一定要这样指定，不能漏了类名。
									std::bind(&Scheduler::ThreadRun, this),
									cppsvr::StrFormat("Thread_%d", i));
		}
		for (int i = 0; i < m_iThreadNum; i++) {
			m_vecThreadPool[i]->Join();
		}
	}

private:
	void ThreadRun() {
		while (true) { // 注意这样设计很不合理！！当任务队列为空的时候就是一直空跑。。
			std::function<void()> oTask = nullptr;
			{
				cppsvr::Mutex::ScopedLock oTaskListLock(m_oTaskListMutex);
				// 抢到的时候可能已经空了，其实即使你在锁之前判断一下也有可能出现这种情况
				if (m_listTasks.size()) {
					oTask = m_listTasks.front();
					m_listTasks.pop_front();
				} else {
					break;
				}
			}
			if (oTask != nullptr) {
				oTask();
			}
		}
	}

public:
	uint32_t m_iThreadNum;
	std::vector<cppsvr::Thread::ptr> m_vecThreadPool;
	std::list<std::function<void()>> m_listTasks;
	cppsvr::Mutex m_oTaskListMutex;
};

uint64_t iCounter = 0;

void TestFunc(std::string sTaskName) {
	TEMP_ASYN_LOG("MSG: one task start. task name %s", sTaskName.c_str());
	cppsvr::SleepMs(50);
	auto iNow = cppsvr::GetCurrentTimeMs();
	// 用下面这种方法测试，如果线程被调度走了，时间也算在里面了。。所以是错的。
	// while (cppsvr::GetCurrentTimeMs() - iNow < iMsCount) {}
	for (int i = 0; i < 100000000; i++);
	TEMP_ASYN_LOG("MSG: one task end. task name %s", sTaskName.c_str());
}

int main() {
	cppsvr::Thread::SetThreadName("main");
	srand(time(nullptr));
	// 我是四核的，每个线程设计基本上算是 sleep 时间和 cpu 占用时间各 50 ms ？
	Scheduler oScheduler(8);
	for (int i = 0; i < 50; i++) {
		oScheduler.AddTask(std::bind(TestFunc, cppsvr::StrFormat("Task_%d", i)));
	}
	TEMP_ASYN_LOG("run threads begin");
	uint64_t iNow = cppsvr::GetCurrentTimeMs();
	oScheduler.Run();
	TEMP_ASYN_LOG("run threads end. cost %d ms", (int)(cppsvr::GetCurrentTimeMs() - iNow));
	
	return 0;
}