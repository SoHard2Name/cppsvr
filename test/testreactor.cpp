#include "cppsvr/cppsvr.h"
#include "ctime"
#include "string"

int main() {
	srand(time(nullptr));
	cppsvr::Thread::SetThreadName("main");
	cppsvr::Reactor oTestReactor("TestReactor");
	int iTestPipeFds[2];
	assert(!pipe(iTestPipeFds));
	oTestReactor.RegisterEvent(iTestPipeFds[0], cppsvr::Reactor::READ_EVENT, [&](){
		char buf[256] = {};
		assert(read(iTestPipeFds[0], buf, 256) > 0);
		DEBUG("test pipe readable, content %s", buf);
		oTestReactor.WakeUp();
	});
	cppsvr::Thread oTestThread([&](){
		int iTimes = 3;
		while(iTimes--) {
			cppsvr::SleepMs(rand() % 2000);
			std::string sTemp;
			for (int i = 0, num = rand() % 10; i < num; i++) {
				sTemp += rand() % 26 + 'a';
			}
			DEBUG("write sth to test pipe: %s", sTemp.c_str());
			write(iTestPipeFds[1], sTemp.c_str(), sTemp.size());
		}
		oTestReactor.UnregisterEvent(iTestPipeFds[0], cppsvr::Reactor::READ_EVENT);
	}, "TestThread");
	oTestReactor.MainLoop();
	
	return 0;
}

/*

2024-07-02 12:25:04 202 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/thread.cpp:26 thread TestThread be constructed succ.
2024-07-02 12:25:04 203 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:04 838 (142719,TestThread; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/test/testreactor.cpp:25 write sth to test pipe: hvy
2024-07-02 12:25:04 839 (142718,main; 2) [DEBUG] /home/abcpony/QQMail/cppsvr/test/testreactor.cpp:14 test pipe readable, content hvy
2024-07-02 12:25:04 840 (142718,main; 2) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:177 wake up reactor TestReactor

// 为什么下面会有这个“one new round”再是被唤醒？ 因为是我上面的测试管道一端可读了，此时
// reactor 上就我这个测试的事件，然后在这个测试事件的回调函数里面再去唤醒 reactor 的。
// 所以它是下一轮循环的时候才被 wake up。

2024-07-02 12:25:04 840 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:04 840 (142718,main; 1) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:181 reactor TestReactor has been waken up.
2024-07-02 12:25:04 841 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:05 590 (142719,TestThread; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/test/testreactor.cpp:25 write sth to test pipe: l
2024-07-02 12:25:05 590 (142718,main; 2) [DEBUG] /home/abcpony/QQMail/cppsvr/test/testreactor.cpp:14 test pipe readable, content l
2024-07-02 12:25:05 591 (142718,main; 2) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:177 wake up reactor TestReactor
2024-07-02 12:25:05 591 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:05 591 (142718,main; 1) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:181 reactor TestReactor has been waken up.
2024-07-02 12:25:05 591 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:07 107 (142719,TestThread; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/test/testreactor.cpp:25 write sth to test pipe: govciqk
2024-07-02 12:25:07 107 (142719,TestThread; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/coroutine.cpp:87 one main coroutine destruct succ.
2024-07-02 12:25:10 593 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:15 599 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round
2024-07-02 12:25:20 605 (142718,main; 0) [DEBUG] /home/abcpony/QQMail/cppsvr/cppsvr/reactor.cpp:55 reactor TestReactor MainLoop one new round

*/