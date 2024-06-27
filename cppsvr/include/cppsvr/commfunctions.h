#pragma once

#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"

namespace cppsvr {


std::string StrFormat(const char *format, ...)__attribute__((format (printf, 1, 2)));

std::string StrFormat(const char *format, va_list args);

pid_t GetThreadId();

void SleepMs(uint32_t milliseconds);

std::string GetTimeNow();

// 去除前后空格
std::string Trim(std::string sStr);

// 获取当前时间戳（毫秒级）
// 返回的时间是从 Unix 纪元（1970年1月1日00:00:00 UTC）到当前时间的微秒数
long long GetCurrentTimeMs();

}