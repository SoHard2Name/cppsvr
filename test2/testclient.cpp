#include "cppsvr/cppsvr.h"
#include "cstring"

int iSuccCount = 0, connsucc = 0;
int iFailCount = 0, connerr = 0;


class TestClient : public cppsvr::CoroutinePool {
	RUN_FUNC_DECL(TestClient);
public:
	TestClient() : CoroutinePool() {
		
	}

	void InitCoroutines() {
		assert(m_iCoroutineNum >= 2);
		m_vecCoroutine[1] = new cppsvr::Coroutine(Report);
		m_vecCoroutine[1]->SwapIn();
		for (int i = 2; i < m_iCoroutineNum; i++) {
			m_vecCoroutine[i] = new cppsvr::Coroutine(ClientCoroutine);
			m_vecCoroutine[i]->SwapIn();
		}
	}
	
private:

	static void ClientCoroutine() {
		DEBUG(".....???");
		// 创建 fd 应当放外边，否则里面要是慢一点，比如处于进行中状态，然后你就循环创建 fd，更容易寄掉。。
		int iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		assert(iFd >= 0);
		cppsvr::SetNonBlock(iFd);
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(1335);
		while (true) {
			int iRet = connect(iFd, (struct sockaddr *)&addr, sizeof(sockaddr));
			// errno == EALREADY 表示已经创建完连接了。
			// errno == EINPROGRESS 表示连接正在建立中。
			if (iRet && errno != EALREADY) {
				if (errno != EINPROGRESS) {
					connerr++;
					ERROR("conn fail.. ret %d fd %d. errno %d, errmsg %s", iRet, iFd, errno, strerror(errno));
				} else {
					INFO("connecting... fd %d", iFd);
				}
				WaitFdEventWithTimeout(-1, -1, 100);
				continue;
			} else {
				connsucc++;
			}
			INFO("conn succ. fd %d", iFd);
			// 每100ms请求一次
			cppsvr::Timer::GetThis()->AddRelativeTimeEvent(10, nullptr, 
				std::bind(CoroutinePool::DefaultProcess, cppsvr::Coroutine::GetThis()), 100);
			DEBUG("what???");
			while (true) {
				cppsvr::Coroutine::GetThis()->SwapOut();
				std::string sReq = cppsvr::UInt2ByteStr(1u) + "World", sResp = "";
				int iRet = cppsvr::ServerCoroutinePool::Write(iFd, sReq);
				if (iRet != 0) {
					ERROR("write req error. ret %d, fd %d", iRet, iFd);
					break;
				}
				iRet = cppsvr::ServerCoroutinePool::Read(iFd, sResp);
				if (iRet != 0) {
					ERROR("read rsp error. ret %d, fd %d", iRet, iFd);
					break;
				}
				DEBUG("TEST: this time req end, req [%s], resp [%s]", sReq.c_str(), sResp.c_str());
				if (sResp == "Hello World") {
					iSuccCount++;
				} else {
					iFailCount++;
				}
			}
		}
	}
	
	
	// 后续把这个重复干事的东西封装起来。
	static void Report() {
		// 每秒报告一次
		cppsvr::Timer::GetThis()->AddRelativeTimeEvent(10, nullptr, 
			std::bind(CoroutinePool::DefaultProcess, cppsvr::Coroutine::GetThis()), 1000);
		while (true) {
			cppsvr::Coroutine::GetThis()->SwapOut();
			std::cout << cppsvr::StrFormat("succ count %d, fail count %d. succ conn %d, fail conn %d",
						 iSuccCount, iFailCount, connsucc, connerr) << std::endl;
		}
	}
	
};

RUN_FUNC_IMPL(TestClient);


int main() {
	TestClient().Run();
	
	return 0;
}