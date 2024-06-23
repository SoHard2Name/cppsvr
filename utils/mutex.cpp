#include "utils/mutex.h"

namespace utility {

Semaphore::Semaphore(uint32_t iCount/* = 0*/) {
	if (sem_init(&m_semaphore, 0, iCount)) { // 0 表示不与其他进程共享
		throw std::logic_error("sem_init error");
	}
}

Semaphore::~Semaphore() {
	sem_destroy(&m_semaphore);
}

void Semaphore::Wait() {
	if (sem_wait(&m_semaphore)) {
		throw std::logic_error("sem_wait error");
	}
}

void Semaphore::Notify() {
	if (sem_post(&m_semaphore)) {
		throw std::logic_error("sem_post error");
	}
}

}