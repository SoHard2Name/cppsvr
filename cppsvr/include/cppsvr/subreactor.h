#pragma once
#include "logger.h"
#include "commfunctions.h"
#include "thread.h"

// 子 reactor 负责处理主 reactor 交给它的连接，这些连接产生的事件就都注册到这个 reactor 上面，但是具体的业务任务可以由线程池中的
// 不同工作线程去完成。子 reactor 负责和多个工作线程交互，以快速完成一个业务，并且直接执行和客户端交流的操作，否则由于多个工作线程
// 同时读写一个 socket 会出问题。

namespace cppsvr {

class SubReactor : public Thread {
public:
	
	
private:
	
	
};

}