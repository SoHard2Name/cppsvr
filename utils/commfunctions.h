#pragma once

#include "string"
#include "memory"
#include "cstdarg"
#include "unistd.h"
#include "sys/syscall.h"

namespace utility {

std::string StrFormat(const char *format, ...);

long GetPid();

void SleepMs(uint32_t milliseconds);

std::string GetTimeNow();

}