#pragma once

#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"

namespace utility {

std::string StrFormat(const char *format, ...);

std::string StrFormat(const char *format, va_list args);

long GetPid();

void SleepMs(uint32_t milliseconds);

std::string GetTimeNow();

// 去除前后空格
std::string Trim(std::string sStr);

}