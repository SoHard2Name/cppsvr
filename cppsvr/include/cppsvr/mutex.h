#pragma once

#include "iostream"
#include "semaphore.h"
#include "atomic"

namespace cppsvr {

// 信号量
class Semaphore {
public:

	Semaphore(uint32_t iCount = 0) {
		if (sem_init(&m_semaphore, 0, iCount)) { // 0 表示不与其他进程共享
			throw std::logic_error("sem_init error");
		}
	}

	~Semaphore() {
		sem_destroy(&m_semaphore);
	}

	void Wait() {
		if (sem_wait(&m_semaphore)) {
			throw std::logic_error("sem_wait error");
		}
	}

	void Notify() {
		if (sem_post(&m_semaphore)) {
			throw std::logic_error("sem_post error");
		}
	}

private:
	Semaphore(const Semaphore &) = delete;
	Semaphore(const Semaphore &&) = delete;
	Semaphore &operator=(const Semaphore &) = delete;

private:
	sem_t m_semaphore;
};

// 模板化，给下面各个锁都弄个退出作用域自动解锁的功能。其中的 m_bLocked
// 都是针对线程对于锁的操作状态，而不是针对锁当前的状态。这些 ScopedLock
// 本质上不是锁，而是用于表示线程去锁住那个锁变量的一个操作。

template <class T>
struct ScopedLockImpl {
public:
	ScopedLockImpl(T &oMutex)
		: m_oMutex(oMutex) {
		m_oMutex.Lock();
		m_bLocked = true;
	}

	~ScopedLockImpl() {
		Unlock();
	}

	void Lock() {
		if (!m_bLocked) {
			m_oMutex.Lock();
			m_bLocked = true;
		}
	}

	void Unlock() {
		if (m_bLocked) {
			m_oMutex.Unlock();
			m_bLocked = false;
		}
	}

private:
	T &m_oMutex;
	bool m_bLocked;
};

template <class T>
struct ScopedReadLockImpl {
public:
	ScopedReadLockImpl(T &oMutex)
		: m_oMutex(oMutex) {
		m_oMutex.ReadLock();
		m_bLocked = true;
	}

	~ScopedReadLockImpl() {
		Unlock();
	}

	void Lock() {
		if (!m_bLocked) {
			m_oMutex.ReadLock();
			m_bLocked = true;
		}
	}

	void Unlock() {
		if (m_bLocked) {
			m_oMutex.Unlock();
			m_bLocked = false;
		}
	}

private:
	T &m_oMutex;
	bool m_bLocked;
};

template <class T>
struct ScopedWriteLockImpl {
public:
	ScopedWriteLockImpl(T &oMutex)
		: m_oMutex(oMutex) {
		m_oMutex.WriteLock();
		m_bLocked = true;
	}

	~ScopedWriteLockImpl() {
		Unlock();
	}

	void Lock() {
		if (!m_bLocked) {
			m_oMutex.WriteLock();
			m_bLocked = true;
		}
	}

	void Unlock() {
		if (m_bLocked) {
			m_oMutex.Unlock();
			m_bLocked = false;
		}
	}

private:
	T &m_oMutex;
	bool m_bLocked;
};

class Mutex {
public:
	typedef ScopedLockImpl<Mutex> ScopedLock;
	Mutex() {
		pthread_mutex_init(&m_oMutex, nullptr);
	}

	~Mutex() {
		pthread_mutex_destroy(&m_oMutex);
	}

	void Lock() {
		pthread_mutex_lock(&m_oMutex);
	}

	void Unlock() {
		pthread_mutex_unlock(&m_oMutex);
	}

private:
	pthread_mutex_t m_oMutex;
};

class RWMutex {
public:
	typedef ScopedReadLockImpl<RWMutex> ScopedReadLock;
	typedef ScopedWriteLockImpl<RWMutex> ScopedWriteLock;

	RWMutex() {
		pthread_rwlock_init(&m_oLock, nullptr);
	}

	~RWMutex() {
		pthread_rwlock_destroy(&m_oLock);
	}

	void ReadLock() {
		pthread_rwlock_rdlock(&m_oLock);
	}

	void WriteLock() {
		pthread_rwlock_wrlock(&m_oLock);
	}

	void Unlock() {
		pthread_rwlock_unlock(&m_oLock);
	}

private:
	pthread_rwlock_t m_oLock;
};

class SpinLock {
public:
	typedef ScopedLockImpl<SpinLock> ScopedLock;
	SpinLock() {
		pthread_spin_init(&m_oMutex, 0);
	}

	~SpinLock() {
		pthread_spin_destroy(&m_oMutex);
	}

	void Lock() {
		pthread_spin_lock(&m_oMutex);
	}

	void Unlock() {
		pthread_spin_unlock(&m_oMutex);
	}

private:
	pthread_spinlock_t m_oMutex;
};

// 这东西其实就是一个自旋锁。不过这个版本不止在 linux 上能用。
class CASLock {
public:
	typedef ScopedLockImpl<CASLock> ScopedLock;
	CASLock() {
		m_oMutex.clear();
	}
	
	~CASLock() {
	}

	// memory_order_acquire 保证该线程的此操作和之后的所有读写操作重排，
	// 之后的操作一定不排到它前面，也就是锁之后才会读写。
	void Lock() {
		m_oMutex.test_and_set(std::memory_order_acquire);
	}

	// memory_order_release 保证该线程的此操作和之前的所有读写操作重排，
	// 之前的操作一定不排到它后面，也就是锁之前的读写都要完成。
	void Unlock() {
		m_oMutex.clear(std::memory_order_release);
	}

private:
	volatile std::atomic_flag m_oMutex;
};

}