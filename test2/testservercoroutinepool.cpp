#include "cppsvr/cppsvr.h"

void TestService(const std::string &sReq, std::string &sResp) {
	sResp = "Hello " + sReq;
}

int main() {
	cppsvr::ServerCoroutinePool::RegisterService(1, TestService);
	auto oTestServerCoroutinePool = cppsvr::ServerCoroutinePool(50);
	auto oTestServerCoroutinePool2 = cppsvr::ServerCoroutinePool(50);
	oTestServerCoroutinePool.Run();
	oTestServerCoroutinePool2.Run();
	INFO("it will join, so can not be end");
	
	return 0;
}
