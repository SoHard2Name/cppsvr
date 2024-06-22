#pragma once

// 把这个放类定义的第一行，只需要在类外自己定义一个无参构造函数即可。
// 如果不定义构造函数，如果用了 GetSingleton() 则会编译时报错。
#define SINGLETON(ClassName)                          \
public:                                               \
	static ClassName *GetSingleton() {                \
		static ClassName oInstance;                   \
		return &oInstance;                            \
	}                                                 \
private:                                              \
	ClassName();                                      \
	ClassName(const ClassName &) = delete;            \
	ClassName &operator=(const ClassName &) = delete; \
	~ClassName() = default;
