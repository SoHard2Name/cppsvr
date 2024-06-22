#pragma once

#include "iostream"
#include "stdexcept"
#include "string"
using std::string;
#include "fstream"
#include "utils/singleton.h"
#include "sstream"
#include "ctime"
#include "cstdarg"
#include "cassert"

#ifdef WIN32 // windows操作系统
#include <windows.h> // 这里要用一个Sleep函数（传毫秒数）
#else
#include <unistd.h> // 这个是Linux系统的，有一个usleep，传微妙数
#endif

namespace utility {

#define log_debug(format, ...) \
		Loger::GetSingleton()->log(Logger::LOG_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_info(format, ...) \
		Loger::GetSingleton()->log(Logger::LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_warn(format, ...) \
		Loger::GetSingleton()->log(Logger::LOG_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_error(format, ...) \
		Loger::GetSingleton()->log(Logger::LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define log_fatal(format, ...) \
		Loger::GetSingleton()->log(Logger::LOG_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)

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
	void open(const string &filename);
	void close();
	void log(Level level, const char *file, int line, const char *format, ...); // file、line 分别为记录处所在的文件和行号
	void set_level(int level);
	void set_max_size(int bytes);
	void set_console(bool console);

private:
	void rotate(); // 滚动，当文件大小超过 m_max 时备份并清空当前文件再继续记录
	void sleep(int milliseconds);
	void localtime(struct tm *time_info, const time_t *ticks); // 把时间戳转为可视化的时间存入time_info

private:
	string m_filename;
	std::ofstream m_ofs;
	int m_max = 0; // 一个文件的最大长度，以字节为单位（超过这个长度则会备份然后清空再继续记录后续的）  如果为0则表示不滚动
	int m_len = 0; // 当前文件长度
	int m_level = LOG_DEBUG; // 日志等级，低于这个等级的日志不记录。
	bool m_console = true; // 是否在控制台调试
};

}