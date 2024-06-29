#include "cppsvr/cppsvr.h"
#include "iostream"
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
using namespace std;

const int MAX_EVENTS = 16;
int pipe_fds[2], epoll_fd;

// 好的封装。加一个非阻塞的 flag
void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void epoll_wait_thread() {
	struct epoll_event events[MAX_EVENTS];
	while (true) {
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // -1 无穷等待
		if (nfds == -1) {
			ERROR("epoll_wait");
			return;
		}
		assert(nfds == 1);
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == pipe_fds[0]) {
				char buf[256];
				read(pipe_fds[0], buf, sizeof(buf));
				INFO("thread get wakeup signal. buf content: %s", buf);
				return;
			}
		}
	}
}

int main() {
	
	cppsvr::Thread::SetThreadName("main");
	
	epoll_fd = epoll_create(1);
	if (epoll_fd == -1) {
		ERROR("epoll_create err");
		return 1;
	}

	if (pipe(pipe_fds) == -1) {
		ERROR("pipe err");
		return 1;
	}

	setNonBlocking(pipe_fds[0]);
	setNonBlocking(pipe_fds[1]);

	struct epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	event.data.fd = pipe_fds[0];
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fds[0], &event) == -1) {
		ERROR("epoll_ctl pipe_fds[0] err");
		return 1;
	}

	const int num_threads = 4;
	std::vector<cppsvr::Thread::ptr> threads(num_threads);
	for (int i = 0; i < num_threads; ++i) {
		threads[i] = std::make_shared<cppsvr::Thread>(epoll_wait_thread, cppsvr::StrFormat("Thread_%d", i));
	}

	for (int i = 0; i < num_threads; ++i) {
		write(pipe_fds[1], "T", 1);
	}

	for (auto& t : threads) {
		t->Join();
	}

	close(pipe_fds[0]);
	close(pipe_fds[1]);
	close(epoll_fd);

	return 0;
}