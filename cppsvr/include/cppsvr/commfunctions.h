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
// 返回的时间是从 Unix 纪元（1970年1月1日00:00:00 UTC）到当前时间的微秒数。
// 注意这里面虽然涉及系统调用，但在 x86_64 系统中和普通的系统调用不同，这个开销特别小的。
uint64_t GetCurrentTimeMs();

uint64_t GetCurrentTimeUs();

// 设置 fd 为非阻塞
void SetNonBlock(int iFd);

uint32_t ByteStr2UInt(const std::string &sStr);
std::string UInt2ByteStr(uint32_t iNum);

}