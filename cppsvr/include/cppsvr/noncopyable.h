#pragma once

#define NON_COPY_ABLE(ClassName)      \
private:                              \
	ClassName(const ClassName &) = delete;  \
	ClassName(const ClassName &&) = delete; \
	ClassName &operator=(const ClassName &) = delete
