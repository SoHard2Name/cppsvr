#include "utils/logger.h"
#include "utils/commfunctions.h"

namespace utility {

// 这个在创建、读配置的时候是不能输出日志的！！
Logger::LoggerConfig::LoggerConfig(std::string sFileName) : ConfigBase(sFileName, false) {
	GetNodeValue(m_oRootNode["FileName"], "", m_sFileName);
	GetNodeValue(m_oRootNode["MaxSize"], 0u, m_iMaxSize);
	GetNodeValue(m_oRootNode["Console"], true, m_bConsole);
	std::string sLevel;
	GetNodeValue(m_oRootNode["Level"], "", sLevel);
	m_eLevel = Logger::GetLevel(sLevel);
	std::cout << StrFormat("MSG: get logger conf succ. file name %s, max size %u, "
						   "console %d, level %s", m_sFileName.c_str(), m_iMaxSize,
							m_bConsole, GetLevelName(m_eLevel).c_str()) << std::endl;
}

Logger::Logger() : m_oLoggerConfig("./conf/loggerconfig.yaml") {
	// 获取配置信息
	m_sFileName = m_oLoggerConfig.m_sFileName;
	m_iMaxSize = m_oLoggerConfig.m_iMaxSize;
	m_eLevel = m_oLoggerConfig.m_eLevel;
	m_bConsole = m_oLoggerConfig.m_bConsole;
	
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

Logger::Level Logger::GetLevel(std::string sLevel) {
#define GET_LEVEL(LevelName) return Logger::LOG_##LevelName
	
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