#pragma once

#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"

namespace utility {

std::string StrFormat(const char *format, ...) {
	va_list args; // 可变参数列表的指针
	va_start(args, format); // 让 arg_ptr 指向 format 后面的第一个参数
	int iSize = snprintf(nullptr, 0, format, args);
	std::string sResult(iSize, '\0');
	va_start(args, format);
	snprintf(&sResult[0], iSize + 1, format, args);
	va_end(args); // 用完可变参数列表指针要记得调用这个东西
	return sResult;
}

long GetPid() {
	return syscall(SYS_gettid);
}

}