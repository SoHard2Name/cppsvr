#include "cppsvr/cppsvr.h"
#include "cppsvr/subreactor.h"

void TestService(const std::string &sReq, std::string &sResp) {
	sResp = "Hello " + sReq;
}

int main() {
	cppsvr::SubReactor::RegisterService(1, TestService);
	// cppsvr::SubReactor oTestServerCoroutinePool(50);
	// cppsvr::SubReactor oTestServerCoroutinePool2(50);
	// oTestServerCoroutinePool.Run();
	// oTestServerCoroutinePool2.Run();
	
	cppsvr::MainReactor oMainReactor;
	oMainReactor.Run();
	
	INFO("it will join, so can not be end");
	
	return 0;
}
