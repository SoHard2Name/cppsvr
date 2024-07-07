#include "iostream"


/*

Sum 函数的汇编

  endbr64 
  push   %rbp
  mov    %rsp,%rbp
  mov    %edi,-0x4(%rbp)
  mov    %esi,-0x8(%rbp)
  mov    -0x4(%rbp),%edx
  mov    -0x8(%rbp),%eax
  add    %edx,%eax
  pop    %rbp
  retq   

*/

int Sum(int a, int b) {
	return a + b;
}

int main() {
	int x = 10;
	int y = 5;
	int z = Sum(x, y);
	std::cout << "sum: " << z << std::endl;
	return 0;
}