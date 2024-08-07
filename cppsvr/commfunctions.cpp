#include "cppsvr/commfunctions.h"
#include "iostream"
#include "cppsvr/logger.h"
#include "sys/time.h"
#include "fcntl.h"
#include "cassert"
#include "cstring"

namespace cppsvr {

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
	if (iSize <= 0) {
		ERROR("StrFormat failed."); // 除非这个错是初始化日志引起的，则这句无意义。
		return "";
	}
	std::string sResult(iSize, '\0');
	va_copy(args_copy, args);
	vsnprintf(&sResult[0], iSize + 1, format, args_copy);
	va_end(args_copy); // 用完可变参数列表指针要记得调用这个东西
	return sResult;
}

pid_t GetThreadId() {
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

uint64_t GetCurrentTimeMs() {
	struct timeval oTimeVal;
	gettimeofday(&oTimeVal, NULL);
	uint64_t timestamp = oTimeVal.tv_sec * 1000ull + oTimeVal.tv_usec / 1000;
	return timestamp;
}

uint64_t GetCurrentTimeUs() {
	struct timeval oTimeVal;
	gettimeofday(&oTimeVal, NULL);
	uint64_t timestamp = oTimeVal.tv_sec * 1000000ull + oTimeVal.tv_usec;
	return timestamp;
}

void SetNonBlock(int iFd) {
	int iFlag = fcntl(iFd, F_GETFL);
	assert(!fcntl(iFd, F_SETFL, iFlag | O_NONBLOCK));
}

uint32_t ByteStr2UInt(const std::string &sStr) {
	uint32_t iNum = 0;
	memcpy(&iNum, sStr.c_str(), 4);
	return iNum;
}

std::string UInt2ByteStr(uint32_t iNum) {
	char sStr[4];
	memcpy(sStr, &iNum, 4);
	return std::string(sStr, 4);
}


}