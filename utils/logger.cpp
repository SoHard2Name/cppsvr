#include "utils/logger.h"
#include "utils/commfunctions.h"

namespace utility {


// TODO: 把这些参数配置化。
Logger::Logger() : m_sFileName("./logs/cppsvr.log"), m_iMaxSize(8192),
		m_eLevel(Logger::LOG_DEBUG), m_bConsole(true) {
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
	return "UNKNOW";
}

void Logger::OpenFile() {
	m_oOutFileStream.open(m_sFileName.c_str(), std::ios::app); // 追加方式
	if (m_oOutFileStream.fail()) {
		throw std::logic_error("open log file failed: " + m_sFileName);
	}
	m_oOutFileStream.seekp(0, std::ios::end); // 把指针放到文件末尾，虽然上面有说ios::app，但是通常会出错。。
	m_iCurrentLen = (int) m_oOutFileStream.tellp();
}

void Logger::CloseFile() {
	m_oOutFileStream.close();
}

void Logger::Log(Logger::Level eLevel, const char *sFile, int iLine, const char *sFormat, ...) {
	if (m_eLevel > eLevel) return; // 等级低于日志等级的消息不做记录
	if (m_oOutFileStream.fail()) {
		throw std::logic_error("open log file failed: " + m_sFileName);
	}
	std::ostringstream oss;
	
	// 时间 [等级] 文件名:行号 
	oss << StrFormat("%s [%s] %s:%d ", GetTimeNow().c_str(), GetLevelName(eLevel).c_str(), sFile, iLine);
	
	va_list pArgList;
	va_start(pArgList, sFormat);
	// 消息内容
	oss << StrFormat(sFormat, pArgList);
	va_end(pArgList);
	
	oss << "\n";
	const std::string &sTemp = oss.str();
	m_oOutFileStream << sTemp;
	m_iCurrentLen += sTemp.length();

	// 这样才能更新到磁盘上，待会一打开文件就能看到最新结果。
	m_oOutFileStream.flush();

	if (m_bConsole) {
		std::cout << sTemp;
	}
	if (m_iMaxSize > 0 && m_iCurrentLen >= m_iMaxSize) {
		Rotate();
	}
}

void Logger::Rotate() {
	CloseFile();
	// 不停 1s 有可能两次封装的文件同名了。只要到时候
	// 把 max size 设置合理就行，不会很影响效率。
	SleepMs(1000);
	time_t iNow = time(nullptr);
	struct tm oNow;
	localtime_r(&iNow, &oNow);
	char sNow[32] = {};
	strftime(sNow, sizeof(sNow), ".%Y%m%d-%H%M%S", &oNow);
	std::string sFileName = m_sFileName + sNow;
	if (rename(m_sFileName.c_str(), sFileName.c_str()) != 0) {
		throw std::logic_error("rename file error: old is " + m_sFileName + ", new is " + sFileName);
	}
	OpenFile();
}

}