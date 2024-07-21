#include "cppsvr/cppsvr.h"


int main() {
	auto oTestServerCoroutinePool = cppsvr::ServerCoroutinePool();
	oTestServerCoroutinePool.Run();
	INFO("it will join, so can not be end");
	
	return 0;
}
