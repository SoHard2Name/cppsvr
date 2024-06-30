#pragma once
#include "logger.h"
#include "commfunctions.h"
#include "thread.h"

// 子 reactor 兼工作线程，它负责处理主 reactor 交给它的连接（每个连
// 接固定到一个 reactor 上面由它处理，否则可能出现多个工作线程一起写
// 入同一个 socket 的情况），这些连接产生的事件就都注册到这个 reactor
// 上面，但是具体的业务任务可以由线程池中的不同工作线程去完成。

namespace cppsvr {

class SubReactor : public Thread {
public:
	
	
private:
	
	
};

}