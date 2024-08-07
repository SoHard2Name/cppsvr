#include "cppsvr/cppsvr.h"

void coroutine(int id) {
	while(true) {
		static int x = 0;
		std::cout << cppsvr::StrFormat("coroutine%d hello world # %d", id, ++x) << std::endl;
		cppsvr::Coroutine::GetThis()->SwapOut();
	}
}

int main() {
	auto pCoroutine1 = new cppsvr::Coroutine(std::bind(coroutine, 1));
	auto pCoroutine2 = new cppsvr::Coroutine(std::bind(coroutine, 2));
	for (int i = 0; i < 3; i++) {
		pCoroutine1->SwapIn();
		pCoroutine2->SwapIn();
	}
	
	return 0;
}

/*

coroutine1 hello world # 1
coroutine2 hello world # 2
coroutine1 hello world # 3
coroutine2 hello world # 4
coroutine1 hello world # 5
coroutine2 hello world # 6

*/