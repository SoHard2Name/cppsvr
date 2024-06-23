#pragma once

#include "iostream"
#include "stdexcept"
#include "string"
#include "fstream"
#include "utils/singleton.h"
#include "sstream"
#include "ctime"
#include "cstdarg"
#include "cassert"

#include <unistd.h>

namespace utility {

// 因为这些宏是在全局用到的，所以就都要加“utility::”
// 至于为什么把 logger 放这层里面？因为直接无命名空间容易影响别人的操作
#define DEBUG(format, ...) \
		utility::Logger::GetSingleton()->Log(utility::Logger::LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define INFO(format, ...) \
		utility::Logger::GetSingleton()->Log(utility::Logger::LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define WARN(format, ...) \
		utility::Logger::GetSingleton()->Log(utility::Logger::LOG_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ERROR(format, ...) \
		utility::Logger::GetSingleton()->Log(utility::Logger::LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define FATAL(format, ...) \
		utility::Logger::GetSingleton()->Log(utility::Logger::LOG_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

class Logger { // 实现为单例模式
	SINGLETON(Logger);

public:
	enum Level {
		LOG_DEBUG = 0,
		LOG_INFO,
		LOG_WARN,
		LOG_ERROR,
		LOG_FATAL,
	};
	static std::string GetLevelName(Level eLevel);
	void Log(Level eLevel, const char *sFile, int iLine, const char *sFormat, ...); // file、line 分别为记录处所在的文件和行号

private:
	void OpenFile();
	void CloseFile();
	void Rotate(); // 滚动，当文件大小超过 m_iMaxSize 时备份并清空当前文件再继续记录

private:
	std::string m_sFileName;
	std::ofstream m_oOutFileStream;
	int m_iMaxSize = 0; // 一个文件的最大长度，以字节为单位（超过这个长度则会备份然后清空再继续记录后续的）  如果为0则表示不滚动
	int m_iCurrentLen = 0; // 当前文件长度
	int m_eLevel = LOG_DEBUG; // 日志等级，低于这个等级的日志不记录。
	bool m_bConsole = true; // 是否在控制台调试
};

}