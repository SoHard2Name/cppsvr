#include "iostream"
#include "string"

/*
   0x0000000000001169 <+0>:     endbr64 
   0x000000000000116d <+4>:     push   %rbp               # 把调用方栈底压入到栈顶
   0x000000000000116e <+5>:     mov    %rsp,%rbp          # 把栈顶指针赋值给栈底指针
   0x0000000000001171 <+8>:     mov    %edi,-0x4(%rbp)    # 把第一个参数值弄到栈上（栈区是从高地址到低地址延伸的）
   0x0000000000001174 <+11>:    addl   $0x1,-0x4(%rbp)    # 参数减一
   0x0000000000001178 <+15>:    mov    -0x4(%rbp),%eax    # 设置返回值，如果函数是没有返回值的不用管这个 %eax 会变成什么，因为是 caller-saved 的
   0x000000000000117b <+18>:    pop    %rbp               # 这个就是把栈顶内容弹出到 %rbp 上，因为是 callee-saved，所以被调用方恢复
   0x000000000000117c <+19>:    retq                      # 。这个其实也就恢复了 %rsp

   # 从上面的汇编语言来看，没有维持 rsp 的值为实际存储数据的栈的最顶部，估计是明确它接下来没有发生函数调用所以就这么弄。
*/
int test(int x)
{
	x++;
	return x;
}

void test2()
{
	int x = 1;
}

extern "C"
{
	extern void test3( void* ) asm("test3");
};


/*
   0x000000000000117d <+0>:     endbr64 
   0x0000000000001181 <+4>:     push   %rbp
   0x0000000000001182 <+5>:     mov    %rsp,%rbp
   0x0000000000001185 <+8>:     sub    $0x20,%rsp           # 栈顶，调用函数就是从这里开始接下去一个栈来的。
   0x0000000000001189 <+12>:    mov    %edi,-0x14(%rbp)
   0x000000000000118c <+15>:    mov    %rsi,-0x20(%rbp)
   0x0000000000001190 <+19>:    mov    $0x1,%edi            # 为将要调用的做准备
   0x0000000000001195 <+24>:    callq  0x1169 <_Z4testi>    # 调用 test 函数，对应上面代码段起点 0x0000000000001169
   0x000000000000119a <+29>:    mov    %eax,-0x4(%rbp)
   0x000000000000119d <+32>:    mov    $0x0,%eax
   0x00000000000011a2 <+37>:    leaveq 
   0x00000000000011a3 <+38>:    retq
*/
int main(int argc, char *argv[]) {
	int ret = test(1);
	void *x;
	test3(x);
	std::cout << "回到这里" << std::endl;
	return 0;
}