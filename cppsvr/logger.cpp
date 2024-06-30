#include "cppsvr/logger.h"
#include "cppsvr/commfunctions.h"
#include "cppsvr/cppsvrconfig.h"
#include "cppsvr/thread.h"
#include "cppsvr/coroutine.h"

namespace cppsvr {


Logger::Logger() {
	// 获取配置信息
	const auto &oConfig = *CppSvrConfig::GetSingleton();
	m_sFileName = oConfig.GetLogFileName();
	m_iMaxSize = oConfig.GetLogMaxSize();
	m_eLevel = oConfig.GetLogLevelValue();
	m_sLevel = oConfig.GetLogLevelName();
	m_bConsole = oConfig.GetLogConsole();
	
	OpenFile();
}

Logger::~Logger() {
	CloseFile();
}

std::string Logger::GetLevelName(Logger::Level eLevel) {

#define GET_NAME(LevelName) \
	if (eLevel == Logger::LOG_##LevelName) return #LevelName
	
	GET_NAME(DEBUG);
	GET_NAME(INFO);
	GET_NAME(WARN);
	GET_NAME(ERROR);
	GET_NAME(FATAL);
	
#undef GET_NAME
	std::cout << StrFormat("%s:%d ERR: unknow level value %d", 
		__FILE__, __LINE__, eLevel) << std::endl;
	return "UNKNOW";
}

Logger::Level Logger::GetLevelValue(std::string sLevel) {
#define GET_LEVEL(LevelName) if(sLevel == #LevelName) return Logger::LOG_##LevelName
	
	GET_LEVEL(DEBUG);
	GET_LEVEL(INFO);
	GET_LEVEL(WARN);
	GET_LEVEL(ERROR);
	GET_LEVEL(FATAL);	

#undef GET_LEVEL
	std::cout << StrFormat("%s:%d ERR: unknow level name %s", 
		__FILE__, __LINE__, sLevel.c_str()) << std::endl;
	return Logger::LOG_UNKNOW;
}

void Logger::OpenFile() {
	m_oOutFileStream.open(m_sFileName.c_str(), std::ios::app); // 追加方式
	if (m_oOutFileStream.fail()) {
		throw std::logic_error(StrFormat("open log file failed. file name %s", m_sFileName.c_str()));
	}
	m_oOutFileStream.seekp(0, std::ios::end); // 把指针放到文件末尾，虽然上面有说ios::app，但是通常会出错。。
	m_iCurrentLen = (int) m_oOutFileStream.tellp();
}

void Logger::CloseFile() {
	m_oOutFileStream.close();
}

void Logger::Log(Logger::Level eLevel, const char *sFile, int iLine, const char *sFormat, ...) {
	if (m_eLevel > eLevel) return; // 等级低于日志等级的消息不做记录
	assert(!m_oOutFileStream.fail());
	std::ostringstream oss;
	
	// 时间 毫秒数 (线程id,自定义线程名; 协程号) [等级] 文件名:行号 
	oss << StrFormat("%s %d (%d,%s; %lu) [%s] %s:%d ", GetTimeNow().c_str(), (int)(GetCurrentTimeMs() % 1000), GetThreadId(),
					Thread::GetThreadName().c_str(), Coroutine::GetThis()->GetId(), GetLevelName(eLevel).c_str(), sFile, iLine);
	
	va_list pArgList;
	va_start(pArgList, sFormat);
	// 消息内容
	oss << StrFormat(sFormat, pArgList);
	va_end(pArgList);
	
	oss << "\n";
	const std::string &sTemp = oss.str();

	{ // 写进日志文件
		Mutex::ScopedLock oLogLock(m_oMutex);
		m_oOutFileStream << sTemp;
		m_iCurrentLen += sTemp.length();

		// 这样才能更新到磁盘上，待会一打开文件就能看到最新结果。
		m_oOutFileStream.flush();

		if (m_iMaxSize > 0 && m_iCurrentLen >= m_iMaxSize) {
			Rotate();
		}
	}
	
	if (m_bConsole) {
		std::cout << sTemp;
	}
}

void Logger::Rotate() { // 这个函数外已经有加锁了。
	CloseFile();
	// 不停 1ms 有可能两次封装的文件同名了。但不会很影响效率。
	SleepMs(1);
	time_t iNow = time(nullptr);
	struct tm oNow;
	localtime_r(&iNow, &oNow);
	char sNow[32] = {};
	strftime(sNow, sizeof(sNow), "%Y%m%d-%H%M%S", &oNow);
	std::string sFileName = StrFormat("%s.%s_%d", m_sFileName.c_str(), sNow, (int)(GetCurrentTimeMs() % 1000));
	if (rename(m_sFileName.c_str(), sFileName.c_str()) != 0) {
		throw std::logic_error(StrFormat("rename file error: old is %s, new is %s", m_sFileName.c_str(), sFileName.c_str()));
	}
	OpenFile();
}

}