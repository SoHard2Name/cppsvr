#pragma once

#include "semaphore.h"
#include "iostream"

namespace utility {

class Semaphore {
public:
	Semaphore(uint32_t iCount = 0);
	~Semaphore();

	void Wait();
	void Notify();

private:
	Semaphore(const Semaphore &) = delete;
	Semaphore(const Semaphore &&) = delete;
	Semaphore &operator=(const Semaphore &) = delete;

private:
	sem_t m_semaphore;
};


}