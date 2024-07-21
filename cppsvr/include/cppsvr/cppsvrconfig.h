#pragma once

#include "configbase.h"
#include "logger.h"

namespace cppsvr {

class CppSvrConfig : public ConfigBase {
public:
	static CppSvrConfig* GetSingleton() {
		static CppSvrConfig oCppSvrConfig("./conf/cppsvrconfig.yaml");
		return &oCppSvrConfig;
	}

	void LogConfigInfo() const;

	const std::string &GetLogFileName() const;
	int GetLogMaxSize() const;
	Logger::Level GetLogLevelValue() const;
	bool GetLogConsole() const;
	const std::string &GetLogLevelName() const;

	uint32_t GetThreadNum() const;
	uint32_t GetCoroutineNum() const;
	uint32_t GetCoroutineStackSize() const;
	
	const std::string &GetIp() const;
	uint32_t GetPort() const;
	bool GetReuseAddr() const;

private:
	CppSvrConfig(std::string sFileName);
	~CppSvrConfig() override {}
	CppSvrConfig(const CppSvrConfig&) = delete;
	CppSvrConfig& operator=(const CppSvrConfig&) = delete;

private:
	std::string m_sLogFileName; // 日志输出的位置
	int m_iLogMaxSize; // 一个文件的最大长度，以字节为单位（超过这个长度则会备份然后清空再继续记录后续的）  如果为0则表示不滚动
	bool m_bLogConsole; // 是否在控制台调试
	Logger::Level m_eLogLevel; // 日志等级，低于这个等级的日志不记录。
	std::string m_sLogLevel; // 日志等级的名称

	uint32_t m_iThreadNum; // （工作）线程数量。
	uint32_t m_iCoroutineNum; // 一个工作线程中的协程数量。
	uint32_t m_iCoroutineStackSize; // 协程栈大小（byte 数）
	
	std::string m_sIp; // IP 地址
	uint32_t m_iPort; // 服务端口
	bool m_bReuseAddr; // 是否复用地址
};


}