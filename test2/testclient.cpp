#include "cppsvr/cppsvr.h"
#include "cstring"

int iSuccCount = 0, connsucc = 0;
int iFailCount = 0, connerr = 0;


class TestClient : public cppsvr::CoroutinePool {
public:
	TestClient() : CoroutinePool(200) {
	}
	~TestClient() {
		MUST_WAIT_THREAD_IN_EVERY_SON_CLASS_DESTRCUTOR_FIRST_LINE
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
			// // 每100ms请求一次
			// auto pTimeEvent = cppsvr::Timer::GetThis()->AddRelativeTimeEvent(10, nullptr, 
			// 	std::bind(CoroutinePool::DefaultProcess, cppsvr::Coroutine::GetThis()), 100);
			// ↑ 错误的，任何时刻不允许有两个或以上的切入协程的事件存在！！！
			// ERROR("what???");
			while (true) {
				// cppsvr::Coroutine::GetThis()->SwapOut();
				// 这样的话没问题，因为每时每刻只会有一个
				cppsvr::CoroutinePool::GetThis()->WaitFdEventWithTimeout(-1, -1, 100);
				std::string sReq = cppsvr::UInt2ByteStr(1u) + "World", sResp = "";
				int iRet = cppsvr::CoroutinePool::Write(iFd, sReq);
				if (iRet != 0) {
					ERROR("write req error. ret %d, fd %d", iRet, iFd);
					break;
				}
				iRet = cppsvr::CoroutinePool::Read(iFd, sResp, 1000);
				if (iRet != 0) {
					ERROR("read resp error. ret %d, fd %d", iRet, iFd);
					iFailCount++;
					close(iFd);
					// cppsvr::Timer::GetThis()->DeleteTimeEvent(pTimeEvent);
					// ERROR("??? why core");
					return;
				}
				// ERROR("TEST: this time req end, req [%s], resp [%s]", sReq.c_str(), sResp.c_str());
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


int main() {
	TestClient().Run();
	
	return 0;
}