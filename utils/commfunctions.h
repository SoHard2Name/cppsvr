#pragma once

#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"

namespace utility {


std::string StrFormat(const char *format, ...)__attribute__((format (printf, 1, 2)));

std::string StrFormat(const char *format, va_list args);

pid_t GetThreadId();

void SleepMs(uint32_t milliseconds);

std::string GetTimeNow();

// 去除前后空格
std::string Trim(std::string sStr);

// 获取当前时间戳（毫秒级）
// 但是要注意这个是 timeofday 的，也就是只是同一天内的一段
// 跨度才能用这个计算。通常就是用于计算一个函数的运行时长。
long long GetCurrentTimeMs();

}