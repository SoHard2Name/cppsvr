#include "iostream"
#include "ucontext.h"

// 下面的 x 是一个栈空间的变量（创建在内存）如果直接正常编译，
// 跑起来之后会发现 x 是会一直增大的，说明 x 并没有被放入寄存
// 器，回到 ctx 的时候实际上是根据栈指针才去找到 x 的值，所以
// 是被修改过的 x 值。

// 但是如果你用 g++ -O2 -o test test.cpp 来编译，就会发现 x 
// 会一直是 1，原因就是 x 被放到寄存器上面去了，每次回到 ctx
// 的时候会恢复寄存器的状态。

// 变量具体有没有被放入寄存器一般是根据编译器来定的，但是正常
// 用协程不会出现这种问题，因为毕竟不需要两个协程运行在同一个
// 函数内，虽然不是并行的。

int main() {
	int x = 0;
	ucontext_t ctx;
	getcontext(&ctx); // 会获取当前所有寄存器的状态。包括栈指针。
	std::cout << ++x << std::endl;
	setcontext(&ctx);
	
	return 0;
}