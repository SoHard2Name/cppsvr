#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"
#include "iostream"

namespace utility {

std::string StrFormat(const char *format, ...) {
	std::cout << "DBG: begin StrFormat" << std::endl;
	va_list args; // 可变参数列表的指针
	va_start(args, format); // 让 arg_ptr 指向 format 后面的第一个参数
	int iSize = vsnprintf(nullptr, 0, format, args);
	std::string sResult(iSize, '\0');
	va_start(args, format);
	vsnprintf(&sResult[0], iSize + 1, format, args);
	va_end(args); // 用完可变参数列表指针要记得调用这个东西
	std::cout << "DBG: StrFormat succ. result " << sResult << std::endl;
	return sResult;
}

long GetPid() {
	return syscall(SYS_gettid);
}

void SleepMs(uint32_t milliseconds) {
	usleep(milliseconds * 1000);
}

std::string GetTimeNow() {
	time_t iNow = time(nullptr);
	struct tm time_info = {};
	localtime_r(&iNow, &time_info);
	char timestamp[32] = {};
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &time_info);
	return std::string(timestamp);
}

}