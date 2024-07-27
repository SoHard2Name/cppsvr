#include "cppsvr/cppsvr.h"
#include "cppsvr/subreactor.h"

void TestService(const std::string &sReq, std::string &sResp) {
	sResp = "Hello " + sReq;
}

int main() {
	cppsvr::SubReactor::RegisterService(1, TestService);
	cppsvr::MainReactor oMainReactor;
	oMainReactor.Run(true);

	return 0;
}
