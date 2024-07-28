- [高性能 C++ 服务器](#高性能-c-服务器)
	- [特别声明](#特别声明)
	- [实现了什么](#实现了什么)
		- [日志器](#日志器)
		- [配置器](#配置器)
		- [线程类封装](#线程类封装)
		- [多协程](#多协程)
			- [协程](#协程)
			- [协程池](#协程池)
				- [定时器](#定时器)
				- [协程信号量](#协程信号量)
		- [主从 reactor 模型](#主从-reactor-模型)
			- [解决多线程日志冲突](#解决多线程日志冲突)
		- [通信协议](#通信协议)
	- [接下来会怎么延伸](#接下来会怎么延伸)
		- [protobuf](#protobuf)
		- [docker](#docker)
		- [...](#)

# 高性能 C++ 服务器

## 特别声明

github 上的 [`EscapeFromEarth`](https://github.com/EscapeFromEarth)、[`SoHard2Name`](https://github.com/SoHard2Name)、[`abcpony`](https://github.com/abcpony)，都是我本人，就是偶尔换来换去而已，项目是本人独立完成的。

作者 `QQ`：`2490273063`

## 实现了什么

### 日志器

日志分为多个等级——`DEBUG`、`INFO`、`WARN`、`ERROR`、`FATAL`，结合宏定义实现了 `C` 风格 `printf` 的格式化输出，例如：

```cpp
ERROR("read req error. ret %d, fd %d", iRet, iFd);
```

支持设置只输出高于特定等级的日志；支持设置是否展示到控制台。

### 配置器

基于 `yaml-cpp` 实现了一个配置器基类 [`ConfigBase`](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/configbase.h)，其中通过函数重载实现了多个 `GetNodeValue` 函数用于读取配置项，使用时只需要指定所读配置项的默认值，便会使用对应的函数，并会在获取不到配置项（用户没配置）的情况下使用默认值。

通过继承此基类，可以方便的实现一个配置器用于读取特定配置规则的文件，例如 [`CppSvrConfig`](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/cppsvrconfig.h)，就是用于读取[框架配置文件](https://github.com/SoHard2Name/cppsvr/blob/master/conf/cppsvrconfig.yaml)的。

> 可以大概看下 `CppSvrConfig` 类的构造函数，实际上只需要简单的指定每个配置项所在的位置以及其默认值，就能把内容读取到内存。
> 
> 通常情况下那些 `GetXXX` 成员函数也是没必要的，直接从对象成员变量去读即可，只要有把握不会发生修改。

### 线程类封装

把 `Linux` 提供的 `<pthread.h>` 中比较常用的操作封装了起来，成了 [`Thread` 类](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/thread.h)，创建线程、`join` 操作、维护和获取线程信息等都更方便，例如：

```cpp
// 创建线程，同时指定其运行的函数 Func，以及运行该函数使用的参数 Param，设置线程名为 Thread_1。
auto *pThread = new Thread(std::bind(&Func, Param), "Thread_1");

// 线程不执行完，创建子线程的线程就不能继续执行接下来的事情。
pThread->Join();
```

同时对常用的互斥量、信号量、自旋锁、相关作用域锁都进行了[封装](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/mutex.h)，其中作用域锁使用如下：

```cpp
// 创建互斥量
cppsvr::Mutex oMutex;

// 锁住互斥量
{
	cppsvr::Mutex::ScopedLock oLock(oMutex);
	// 逻辑操作...
} // 出了这里自动解锁
```

### <font color='red'>多协程</font>

#### 协程

实现了 [`Coroutine` 类](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/coroutine.h)，每个协程维护一个栈、必要的寄存器信息，以及所执行的函数和调用它的父协程指针等；用汇编语言实现了 [`CoSwap` 函数](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/coswap.S)，用于切换协程的寄存器信息（包括栈指针，也就是也切换了栈），基于这个函数又封装了 `Coroutine::SwapIn` 和 `Coroutine::SwapOut`，实现协程切换。

```cpp
// 创建一个栈大小为 64kB 的协程
auto *pCoroutine = new Coroutine(std::bind(&Func, Param), 65536);

// 切换到执行 pCoroutine
pCoroutine->SwapIn();

// pCoroutine 切换回父协程
pCoroutine->SwapOut();
```

#### 协程池

每个线程内可以创建一个协程池，协程池中可以添加多个不同类型的协程，当要发生等待时主动切出并把自己挂到 `epoll` 树上。协程池会维护一个就绪协程队列，有一个调度协程，一直循环执行：超时为 `1ms` 的 `epoll_wait` 操作 → 处理就绪的 `epoll` 事件（比如把某一协程放入就绪协程队列并删除对应注册在[定时器](#定时器)上的超时事件）→ 处理定时器上已经超时的事件 → 从头到尾将就绪协程队列中的协程逐一切入执行到其主动切出（期间如果唤醒了其他协程也会把它加到队列末尾，本轮也会执行它）。

##### 定时器

实现了 [`timer` 类](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/timer.h)：毫秒级定时器（配合上面说的 `epoll_wait` 超时为 `1ms` 实现的），维护含有 `60*1000` 个链表的时间轮盘，每个链表中存的是超时时间在同一毫秒的多个事件，每毫秒都会检测并取出处理。

针对超时时间 `> 60s` 的事件，就会把它放到超时相对现在为 `(60*1000-1)ms` 对应的链表中，每次检测的时候遇到超时时间还没到的事件，会给它重新计算并放到对应链表中。

支持注册循环触发的事件，比如每隔 `100ms` 就切入某个协程，让它执行例行的工作。

```cpp
// 这是一个每秒报告一次的协程（函数）
static void Report() {
	// 创建一个 123ms 后开始触发，并且每隔 1000ms 就触发一次的循环触发事件
	cppsvr::Timer::GetThis()->AddRelativeTimeEvent(123, nullptr, 
		// 触发的事件 DefaultProcess 就是切回本协程
		std::bind(CoroutinePool::DefaultProcess, cppsvr::Coroutine::GetThis()), 1000);
	while (true) {
		cppsvr::Coroutine::GetThis()->SwapOut();
		std::cout << cppsvr::StrFormat("succ count %d, fail count %d. succ conn %d, fail conn %d",
						iSuccCount, iFailCount, connsucc, connerr) << std::endl;
	}
}
```

##### 协程信号量

实现了 [`CoSemaphore` 类](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/comutex.h)，其中维护了一个计数器和一个链表，`wait` 的时候会给计数器减一，当减完为负数时会把当前协程添加到链表上并切出；`post` 的时候会给计数器加一，并在加完为非正数情况下把链表上的协程加到就绪协程队列上。

### 主从 reactor 模型

一个[主 `reactor`](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/mainreactor.h)，多个[子 `reactor`](https://github.com/SoHard2Name/cppsvr/blob/master/cppsvr/include/cppsvr/subreactor.h)。

主线程（也是主 `reactor`）专门用于 `listen` 端口并 `accept` 客户端的连接，然后根据各个子 `reactor` 当前未关闭的连接数，把连接分给其中最少的一个，实现负载均衡。

子 `reactor` 可以注册不同的服务函数，其中创建了多个工作协程，当客户端请求的时候就指定想要的服务以及服务请求体，工作协程就会根据对应服务函数进行解析和处理，返回结果给客户端。

#### 解决多线程日志冲突

把日志器改造成了每个线程首先输出到的是自己线程的缓冲区中。

在每个子 `reactor` 协程池中注册一个协程，每隔一段时间就锁住全局缓冲区，并把其线程缓冲区内容合并到全局缓冲区中；而主 `reactor` 协程池中则注册一个协程，每隔一段时间就锁住全局缓冲区，并把全局缓冲区内容合并到其线程缓冲区中，然后再由它来打印到控制台以及输出到文件。这样就解决了冲突并且不会有太多的锁开销。

但是以上还有日志时间排序问题，所以把上面说的“合并”都弄成按微妙级时间戳顺序归并，并且大家相隔时间相同，并且主线程在输出日志的时候只输出距离最近超过这个相隔时间 `+ 100ms` 的内容。以上就保证了日志有序了。

### 通信协议

暂时实现的是非常简单的，`1 ~ 4` 字节是消息长度，`5 ~ 8` 是请求服务的编号，剩下的是请求体/返回体。

## 接下来会怎么延伸

### protobuf

> 之前学习 `protobuf` 反射、实现 `pb` 和 `json` 互转的[记录](https://github.com/EscapeFromEarth/testBazel_Protobuf)。
>
> 当然本项目如果用上 `protobuf`，主要还是用它简单的序列化和反序列化操作。除非后面要弄成 `web` 服务器之类的，需要用到 `json` 时候才会用上上面这套。

使用上使用方便、兼容性超强、性能不错的 `protobuf` 来方便客户端和服务端之间的通信，实现更复杂的服务。

### docker

> 之前学习 `docker` 基础操作的[记录](https://github.com/EscapeFromEarth/test_docker)。

把这些东西用 `docker` 打包起来，方便部署。

### ...