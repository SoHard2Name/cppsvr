#include "ucontext.h"
#include "memory"
#include "cppsvr/cppsvr.h"

ucontext_t oCtxA, oCtxB;

void FuncA() {
	INFO("func a");
	setcontext(&oCtxB);
}

void FuncB() {
	INFO("func b");
}

// 下面这个就大致模仿了 swapcontext 函数的原理。
// 首先要把 ctx a 存起来，但是从 b 返回的时候肯定不想再 set b，
// 否则就死循环了，所以说 swapcontext 的作用就是用于保存 ctx a
// 同时又去执行 ctx b，然后如果执行完 a 返回到 b 则继续执行 swap
// 函数以下的逻辑。如果执行完 a 不返回 b 则直接段错误。
void MySwapCtx(ucontext_t &oCtxA, ucontext_t &oCtxB) {
	bool flag = 0;
	getcontext(&oCtxA);
	if (!flag) {
		flag = true; // 注意这行不能放下面，放下面就没执行到了。
		setcontext(&oCtxB);
	}
}

int main() {
	getcontext(&oCtxA); // 这句是要求必须有的，否则遇到 set 或者 swap 会直接 core。
	// oCtxA.uc_link = &oCtxB; // 这个是设置 ctx a 执行完以后如果 ctx b 不为空就继续执行 ctx b。最下面有例子。
	oCtxA.uc_stack.ss_sp = malloc(8192);
	oCtxA.uc_stack.ss_size = 8192;
	makecontext(&oCtxA, FuncA, 0);
	
	// 下面两个函数一样的效果。
	// swapcontext(&oCtxB, &oCtxA);
	MySwapCtx(oCtxB, oCtxA);
	
	FuncB();
	
	return 0;
}


/*

#include "iostream"
#include "ucontext.h"

ucontext_t oCtxA, oCtxB;

void FuncA() {
	std::cout << "func a" << std::endl;
}

int main() {
	getcontext(&oCtxA); // 这句是要求必须有的，否则遇到 set 或者 swap 会直接 core。
	oCtxA.uc_link = &oCtxB;
	oCtxA.uc_stack.ss_sp = malloc(8192);
	oCtxA.uc_stack.ss_size = 8192;
	makecontext(&oCtxA, FuncA, 0);
	
	swapcontext(&oCtxB, &oCtxA);
	
	std::cout << "succ" << std::endl;
	
	return 0;
}

最终输出是
func a
succ

*/