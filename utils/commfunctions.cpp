#include "utils/commfunctions.h"
#include "iostream"

namespace utility {

std::string StrFormat(const char *format, ...) {
	va_list args; // 可变参数列表的指针
	va_start(args, format); // 让 arg_ptr 指向 format 后面的第一个参数
	return StrFormat(format, args);
}

// 传 va_list 的要让它走下面这套逻辑；否则走上面的逻辑是被解释成传的那个
// args 是一个可变参数，而 args 又只是代表原本可变参数里面的第一个参数。
// 总之就是只会拿到第一个参数。通过这样重载之后就没事了。
std::string StrFormat(const char *format, va_list args) {
	va_list args_copy;
	va_copy(args_copy, args);
	int iSize = vsnprintf(nullptr, 0, format, args_copy);
	if (iSize <= 0) return "";  // 通常不会遇到这种情况，而是有问题直接就 core 掉了。
								// 如果遇到段错误的时候就 gdb 看看是不是在这里面，是
								// 的话就是用了比如 ("%s", 1) 导致的。
	std::string sResult(iSize, '\0');
	va_copy(args_copy, args);
	vsnprintf(&sResult[0], iSize + 1, format, args_copy);
	va_end(args_copy); // 用完可变参数列表指针要记得调用这个东西
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

std::string Trim(std::string sStr) {
	auto iFirst = sStr.find_first_not_of(' ');
	if (iFirst == std::string::npos) {
		return "";
	}
	auto iLast = sStr.find_last_not_of(' ');
	return sStr.substr(iFirst, iLast - iFirst + 1);
}

}