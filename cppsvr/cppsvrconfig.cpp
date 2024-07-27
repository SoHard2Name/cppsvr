#include "cppsvr/cppsvrconfig.h"
#include "cppsvr/logger.h"

namespace cppsvr {

// 别人可以，就是这个框架配置器的初始化不能输出日志，因为这个读出来的东西是给配置用的。。
CppSvrConfig::CppSvrConfig(std::string sFileName) : ConfigBase(sFileName, false) {
	// 日志配置
	const auto &oLoggerNode = m_oRootNode["Logger"];
	GetNodeValue(oLoggerNode["FileName"], "./logs/cppsvr.log", m_sLogFileName);
	GetNodeValue(oLoggerNode["MaxSize"], 4*1024*1024u, m_iLogMaxSize);
	GetNodeValue(oLoggerNode["Console"], true, m_bLogConsole);
	GetNodeValue(oLoggerNode["Level"], "DEBUG", m_sLogLevel);
	m_eLogLevel = Logger::GetLevelValue(m_sLogLevel);
	GetNodeValue(oLoggerNode["FlushInterval"], 10u, m_iFlushInterval);
	
	// 服务器配置
	const auto &oServerNode = m_oRootNode["Server"];
	GetNodeValue(oServerNode["IP"], "0.0.0.0", m_sIp);
	GetNodeValue(oServerNode["Port"], 8888u, m_iPort);
	GetNodeValue(oServerNode["ReuseAddr"], false, m_bReuseAddr);
	GetNodeValue(oServerNode["WorkerThreadNum"], 1u, m_iWorkerThreadNum);
	GetNodeValue(oServerNode["WorkerCoroutineNum"], 100u, m_iWorkerCoroutineNum);
	GetNodeValue(oServerNode["StackSize"], 128*1024u, m_iCoroutineStackSize);
}

// 虽然这里用到了日志器，但是上面已经创建完配置器，所以日志器能正常获取配置信息。
void CppSvrConfig::LogConfigInfo() const {
	INFO("config info: log file name %s, log max size %u, log console %d, "
		 "log level %s[%d], log flush interval %u, ip %s, port %d, reuse addr %d, "
		 "worker thread num %u, worker coroutine num %u, coroutine stack size %u.", 
		 m_sLogFileName.c_str(), m_iLogMaxSize, m_bLogConsole, m_sLogLevel.c_str(),
		 m_eLogLevel, m_iFlushInterval, m_sIp, m_iPort, m_bReuseAddr, 
		 m_iWorkerThreadNum, m_iWorkerCoroutineNum, m_iCoroutineStackSize);
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

uint32_t CppSvrConfig::GetCoroutineStackSize() const{
	return m_iCoroutineStackSize;
}

const std::string &CppSvrConfig::GetIp() const {
	return m_sIp;
}

uint32_t CppSvrConfig::GetPort() const {
	return m_iPort;
}

bool CppSvrConfig::GetReuseAddr() const {
	return m_bReuseAddr;
}

uint32_t CppSvrConfig::GetWorkerThreadNum() const {
	return m_iWorkerThreadNum;
}

uint32_t CppSvrConfig::GetWorkerCoroutineNum() const {
	return m_iWorkerCoroutineNum;
}

const std::string &CppSvrConfig::GetLogLevelName() const {
	return m_sLogLevel;
}

uint32_t CppSvrConfig::GetFlushInterval() {
	return m_iFlushInterval;
}

}