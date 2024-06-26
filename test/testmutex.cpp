#include "fstream"
#include "utils/utility.h"

utility::RWMutex oLogRWMutex; // 日志读写锁
utility::Mutex oConsoleMutex; // 控制台锁

// 读日志并写到控制台
void ReadLog(int iLineNum) {
	std::ifstream ifs("./logs/cppsvr.log");
	std::vector<std::string> vecLogs;
	{
		utility::RWMutex::ScopedReadLock oReadLog(oLogRWMutex);
		while (iLineNum--) {
			std::string sOneLine;
			if (std::getline(ifs, sOneLine)) {
				vecLogs.push_back(std::move(sOneLine));
			} else {
				break;
			}
		}
		sleep(1);
	}
	{
		utility::Mutex::ScopedLock oOut(oConsoleMutex);
		for (const auto &sOneLine : vecLogs) {
			std::cout << "****" << sOneLine << std::endl;
		}
	}
}

// 写日志
void WriteLog(std::string sLog) {
	utility::RWMutex::ScopedWriteLock oWriteLog(oLogRWMutex);
	ERROR(sLog.c_str());
}

int main() {
	
	// 记得要把日志等级调到 ERROR，否则有些地方是多线程的，直接就把同步日志搞乱了。
	
	ERROR("begin main()"); // 这个只是开始的时候，只有单个线程没事。
	long long iBegin = utility::GetCurrentTimeMs();
	utility::Thread oThread1(std::bind(WriteLog, "这样就实现多种格式的形参列表"), "thread1");
	utility::Thread oThread2(std::bind(ReadLog, 2), "thread2");
	utility::Thread oThread3(std::bind(WriteLog, "emm"), "thread3"); // 这个排前面也没用，饥饿。
	utility::Thread oThread4(std::bind(ReadLog, 2), "thread4");
	utility::Thread oThread5(std::bind(ReadLog, 5), "thread5");
	utility::Thread oThread6(std::bind(WriteLog, "666666"), "thread6");

	// 这个批量加入之后再统一 join 也是一个好用的操作，有需要再封装。
	oThread1.Join();
	oThread2.Join();
	oThread3.Join();
	oThread4.Join();
	oThread5.Join();
	oThread6.Join();
	
	// 果然日志和控制台内容都是正常打印，然后耗时 1001ms 。
	ERROR("end main(), use time %lldms", utility::GetCurrentTimeMs() - iBegin);
	
	return 0;
}