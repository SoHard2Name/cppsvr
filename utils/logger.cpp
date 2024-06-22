#include "utils/logger.h"
#include "utils/commfunctions.h"

namespace utility {


std::string Logger::GetLevelName(Logger::Level eLevel) {

#define GetName(LevelName) \
	if (eLevel == Logger::LOG_##LevelName) return #LevelName
	
	GetName(DEBUG);
	GetName(INFO);
	GetName(WARN);
	GetName(ERROR);
	GetName(FATAL);
	
#undef GetName
	return "UNKNOW";
}

void Logger::open(const string &filename) {
	m_filename = filename;
	m_ofs.open(filename.c_str(), std::ios::app); // 追加方式
	if (m_ofs.fail()) {
		throw std::logic_error("open log file failed: " + filename);
	}
	m_ofs.seekp(0, std::ios::end); // 把指针放到文件末尾，虽然上面有说ios::app，但是通常会出错。。
	m_len = (int) m_ofs.tellp();
}

void Logger::close() {
	m_ofs.close();
}

void Logger::log(Logger::Level level, const char *file, int line, const char *format, ...) {
	if (m_level > level)return; // 等级低于日志等级的消息不做记录
	if (m_ofs.fail()) {
		throw std::logic_error("open log file failed: " + m_filename);
	}
	std::ostringstream oss;
	time_t ticks = time(nullptr); // 时间戳
	struct tm time_info = {};
	localtime_r(&ticks, &time_info);
	char timestamp[32] = {};
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H-%M-%S", &time_info); // 日期 时间点
	
	// 时间 [等级] 文件名:行号 
	oss << StrFormat("%s [%s] %s:%d ", timestamp, GetLevelName(level).c_str(), file, line);

	va_list arg_ptr;
	va_start(arg_ptr, format);
	// 日志内容
	oss << StrFormat(format, arg_ptr);
	va_end(arg_ptr);

	oss << "\n";
	const string &str = oss.str();
	m_ofs << str;
	m_len += str.length();

	// 这样才能更新到磁盘上，使我待会一打开文件就能看到最新结果。
	m_ofs.flush();

	if (m_console) {
		std::cout << str;
	}
	if (m_max > 0 && m_len >= m_max) {
		rotate();
	}
}

void Logger::set_level(int level) {
	m_level = level;
}

void Logger::set_console(bool console) {
	m_console = console;
}

void Logger::set_max_size(int bytes) {
	m_max = bytes;
}

void Logger::rotate() {
	close();
	sleep(1000);
	time_t ticks = time(nullptr);
	struct tm time_info = {};
	localtime(&time_info, &ticks);
	char timestamp[32] = {};
	strftime(timestamp, sizeof(timestamp), ".%Y-%m-%d_%H-%M-%S", &time_info);
	string filename = m_filename + timestamp;
	if (rename(m_filename.c_str(), filename.c_str()) != 0) {
		throw std::logic_error("rename file error: old is " + m_filename + ", new is " + filename);
	}
	open(m_filename);
}

void Logger::sleep(int milliseconds) {
	usleep(milliseconds * 1000);
}

void Logger::localtime(struct tm *time_info, const time_t *ticks) {
	localtime_r(ticks, time_info);
}

}