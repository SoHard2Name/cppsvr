#include "utils/logger.h"
#include "utils/commfunctions.h"

namespace utility {


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

void Logger::Open(const string &filename) {
	m_sFileName = filename;
	m_oOutFileStream.Open(filename.c_str(), std::ios::app); // 追加方式
	if (m_oOutFileStream.fail()) {
		throw std::logic_error("open log file failed: " + filename);
	}
	m_oOutFileStream.seekp(0, std::ios::end); // 把指针放到文件末尾，虽然上面有说ios::app，但是通常会出错。。
	m_iLen = (int) m_oOutFileStream.tellp();
}

void Logger::close() {
	m_oOutFileStream.close();
}

void Logger::log(Logger::Level level, const char *file, int line, const char *format, ...) {
	if (m_eLevel > level)return; // 等级低于日志等级的消息不做记录
	if (m_oOutFileStream.fail()) {
		throw std::logic_error("open log file failed: " + m_sFileName);
	}
	std::ostringstream oss;

	// 时间 [等级] 文件名:行号 
	oss << StrFormat("%s [%s] %s:%d ", GetTimeNow().c_str(), GetLevelName(level).c_str(), file, line);

	va_list arg_ptr;
	va_start(arg_ptr, format);
	// 日志内容
	oss << StrFormat(format, arg_ptr);
	va_end(arg_ptr);

	oss << "\n";
	const string &str = oss.str();
	m_oOutFileStream << str;
	m_iLen += str.length();

	// 这样才能更新到磁盘上，使我待会一打开文件就能看到最新结果。
	m_oOutFileStream.flush();

	if (m_bConsole) {
		std::cout << str;
	}
	if (m_iMax > 0 && m_iLen >= m_iMax) {
		rotate();
	}
}

// TODO: 把这些设置配置化。
void Logger::set_level(int level) {
	m_eLevel = level;
}

void Logger::set_console(bool console) {
	m_bConsole = console;
}

void Logger::set_max_size(int bytes) {
	m_iMax = bytes;
}

void Logger::rotate() {
	close();
	SleepMs(1000);
	string filename = m_sFileName + "." + GetTimeNow();
	if (rename(m_sFileName.c_str(), filename.c_str()) != 0) {
		throw std::logic_error("rename file error: old is " + m_sFileName + ", new is " + filename);
	}
	Open(m_sFileName);
}

}