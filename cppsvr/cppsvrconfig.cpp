#include "cppsvr/cppsvrconfig.h"
#include "cppsvr/logger.h"

namespace cppsvr {

// 别人可以，就是这个框架配置器的初始化不能输出日志，因为这个读出来的东西是给配置用的。。
CppSvrConfig::CppSvrConfig(std::string sFileName) : ConfigBase(sFileName, false) {
	// 日志配置
	const auto &oLoggerNode = m_oRootNode["Logger"];
	GetNodeValue(oLoggerNode["FileName"], "", m_sLogFileName);
	GetNodeValue(oLoggerNode["MaxSize"], 0u, m_iLogMaxSize);
	GetNodeValue(oLoggerNode["Console"], true, m_bLogConsole);
	GetNodeValue(oLoggerNode["Level"], "", m_sLogLevel);
	m_eLogLevel = Logger::GetLevelValue(m_sLogLevel);
	
	// 协程配置
	const auto &oFiberNode = m_oRootNode["Fiber"];
	GetNodeValue(oFiberNode["StackSize"], 2*1024*1024u, m_iFiberStackSize);
}

// 虽然这里用到了日志器，但是上面已经创建完配置器，所以日志器能正常获取配置信息。
void CppSvrConfig::LogConfigInfo() const {
	INFO("config info: log file name %s, log max size %u, log console %d, "
		 "log level %s[%d]. fiber stack size %u", m_sLogFileName.c_str(), 
		 m_iLogMaxSize, m_bLogConsole, m_sLogLevel.c_str(),
		 m_eLogLevel, m_iFiberStackSize);
}

const std::string &CppSvrConfig::GetLogFileName() const {
	return m_sLogFileName;
}

int CppSvrConfig::GetLogMaxSize() const{
	return m_iLogMaxSize;
}

Logger::Level CppSvrConfig::GetLogLevelValue() const{
	return m_eLogLevel;
}

bool CppSvrConfig::GetLogConsole() const{
	return m_bLogConsole;
}

uint32_t CppSvrConfig::GetFiberStackSize() const{
	return m_iFiberStackSize;
}

const std::string &CppSvrConfig::GetLogLevelName() const {
	return m_sLogLevel;
}

}