.globl CoSwap                # 声明一个全局可见的符号 CoSwap

.type CoSwap, @function      # 告诉编译器和链接器，符号 CoSwap 是一个函数，到时候调用 CoSwap 就是链接到这里来。

CoSwap:                      # 定义 CoSwap 符号，只考虑 64 位的

    # 这些都是 callee-saved 的，所以得给它存储起来后面恢复，
    # 还有少数其他的，听网上说是几乎不会用到的，所以一般不管。
    movq %rbx, 0(%rdi)
    movq %rsp, 8(%rdi)
    movq %rbp, 16(%rdi)
    movq %r12, 24(%rdi)
    movq %r13, 32(%rdi)
    movq %r14, 40(%rdi)
    movq %r15, 48(%rdi)

    movq 0(%rsi), %rbx
    movq 8(%rsi), %rsp
    movq 16(%rsi), %rbp
    movq 24(%rsi), %r12
    movq 32(%rsi), %r13
    movq 40(%rsi), %r14
    movq 48(%rsi), %r15
    movq 56(%rsi), %rdi      # rdi 本就是 caller-saved 的，但这里由于可能是要调用协程的函数
                             #（并不是在“恢复调用者上下文”），所以还是要设置上 rdi 的，至于
                             # 56(%rsi) 的值调用这个之前自己设置好。
                             # 由于我确定只会有一个参数，所以就只搞了 rdi，其他的不管。

    ret                      # CoSwap 结束，跳到 rsi 上下文对应的位置：
                             # 1. 可能是调用函数，则正好设置好了调用函数的 rsp、rbp、rdi 这些
                             #    上下文，ret 就相当于调用 rsp 指向的那个函数。
                             # 2. 可能是切换上下文，则切入方认为自己从它之前调用的那个 CoSwap
                             #    那里返回，同理下次切回来的时候当前切出方也是认为自己从调用这
                             #    个 CoSwap 返回。

