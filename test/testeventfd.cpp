#include "cppsvr/cppsvr.h"

void wait_thread(int epoll_fd, int efd) {
    struct epoll_event events[1];
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, 1, -1);
        if (nfds == -1) {
			ERROR("epoll_wait error.");
			break;
        }

        if (events[0].data.fd == efd) {
            uint64_t count;
            int bytes_read = read(efd, &count, sizeof(count));
            if (bytes_read == sizeof(count)) {
                std::cout << "Thread " << std::this_thread::get_id() << " received signal, count: " << count << std::endl;
                break; // Exit after receiving the signal
            } else {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                break;
            }
        }
    }
}


int main() {
	
	
	return 0;
}



#include <iostream>
#include <thread>
#include <vector>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>


int main() {
    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "epoll_create1 error" << std::endl;
        return 1;
    }

    // Create an eventfd
    int efd = eventfd(0, 0);
    if (efd == -1) {
        std::cerr << "eventfd error" << std::endl;
        return 1;
    }

    // Add the eventfd to the epoll instance
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = efd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, efd, &ev) == -1) {
        std::cerr << "epoll_ctl error" << std::endl;
        return 1;
    }

    // Start multiple threads that wait on the eventfd
    const int num_threads = 3;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(wait_thread, epoll_fd, efd);
    }

    sleep(1); // Give the threads time to start waiting

    // Uncomment one of the following blocks to test different behaviors

    // Block 1: Wake up one thread
    // uint64_t signal = 1;
    // if (write(efd, &signal, sizeof(signal)) != sizeof(signal)) {
    //     std::cerr << "eventfd write error" << std::endl;
    //     return 1;
    // }

    // Block 2: Broadcast wake up all threads
    uint64_t signal = num_threads;
    if (write(efd, &signal, sizeof(signal)) != sizeof(signal)) {
        std::cerr << "eventfd write error" << std::endl;
        return 1;
    }

    for (auto& t : threads) {
        t.join();
    }

    close(efd);
    close(epoll_fd);

    return 0;
}