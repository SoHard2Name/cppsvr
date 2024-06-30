#pragma once

#define NON_COPY_ABLE(ClassName)      \
private:                              \
	Thread(const Thread &) = delete;  \
	Thread(const Thread &&) = delete; \
	Thread &operator=(const Thread &) = delete
