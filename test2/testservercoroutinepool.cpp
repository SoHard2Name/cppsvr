#include "cppsvr/cppsvr.h"

void TestService(const std::string &sReq, std::string &sResp) {
	sResp = "Hello " + sReq;
}

int main() {
	cppsvr::ServerCoroutinePool::RegisterService(1, TestService);
	auto oTestServerCoroutinePool = cppsvr::ServerCoroutinePool();
	oTestServerCoroutinePool.Run();
	INFO("it will join, so can not be end");
	
	return 0;
}
