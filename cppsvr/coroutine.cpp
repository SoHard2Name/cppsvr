#include "cppsvr/coroutine.h"
#include "cppsvr/cppsvrconfig.h"
#include <atomic>
#include "cppsvr/threadpool.h"
namespace cppsvr {

// 计算线程 id 
static std::atomic<uint64_t> s_iCoroutineId{0};
// 线程总数（包括那些每个线程的主协程，只算存活）
static std::atomic<uint64_t> s_iCoroutineCount{0};

// 线程局部变量（当前协程）
static thread_local Coroutine *t_pCurrentCoroutine = nullptr;
// 指向主协程信息，否则没人维护。
static thread_local Coroutine::ptr t_pMainCoroutine = nullptr;

// 配合配置系统指定协程的 栈大小   暂时就先不改配置的名称。
static uint32_t g_iCoroutineStackSize = CppSvrConfig::GetSingleton()->GetCoroutineStackSize();

// 统一的方法来处理协程栈空间的分配
class MallocStackAllocator {
public:
	// 分配协程栈
	static void *Alloc(size_t iSize) {
		return malloc(iSize);
	}
	// 销毁协程栈（这里提供size来支持未来可能的需要）
	static void Dealloc(void *pStack, size_t iSize) {
		return free(pStack);
	}
};

// 使用了个别名 如果未来需要替换栈分配的方式，这里可以比较方便
using StackAllocator = MallocStackAllocator;

// 将当前线程的上下文交由主协程
Coroutine::Coroutine() : m_pReturnCoroutine(nullptr) {
	m_eState = EXEC;
	SetThis(this);

	// 初始化 context，经过这样设置的后面才能使用。
	GET_CONTEXT(m_oCtx);

	++s_iCoroutineCount;
	
	// 这句不能输出，因为在输出日志的时候就获取不了 GetThis 了
	// DEBUG("one main coroutine construct succ");
}

// 创建子协程
Coroutine::Coroutine(std::function<void()> funCallBack, size_t iStackSize/* = 0*/)
	: m_iId(++s_iCoroutineId), m_eState(Coroutine::INIT), m_funCallBack(funCallBack), m_pReturnCoroutine(nullptr) {
	// 统计数加一
	++s_iCoroutineCount;
	// 栈大小，有设置就用设置的，没设置就用配置文件中的
	m_iStackSize = iStackSize ? iStackSize : g_iCoroutineStackSize;
	// 分配栈内存
	m_pStack = StackAllocator::Alloc(m_iStackSize);
	// 获取当前上下文
	GET_CONTEXT(m_oCtx);
	// 没有关联上下文（如果有关联上下文，自身结束时会到关联上下文执行）
	m_oCtx.uc_link = nullptr;
	// 指定上下文起始指针
	m_oCtx.uc_stack.ss_sp = m_pStack;
	// 指定栈大小
	m_oCtx.uc_stack.ss_size = m_iStackSize;
	// 创建上下文
	makecontext(&m_oCtx, &Coroutine::MainFunc, 0);
	DEBUG("one son coroutine construct succ");
}

Coroutine::~Coroutine() {
	// 统计数减一
	--s_iCoroutineCount;
	if (m_pStack) { // 子协程会有m_stack
		assert(m_eState == TERM || m_eState == INIT);
		// 回收栈
		StackAllocator::Dealloc(m_pStack, m_iStackSize);
		DEBUG("one son coroutine destruct succ.");
	} else { // 主协程不会有m_stack
		assert(!m_funCallBack);
		assert(m_eState == EXEC);

		if (t_pCurrentCoroutine == this) {
			SetThis(nullptr);
		}
		DEBUG("one main coroutine destruct succ.");
	}
}

// 切换到当前协程
void Coroutine::SwapIn() {
	m_oSpinlock.Lock();
	auto pCurrentCoroutine = GetThis();
	m_pReturnCoroutine = pCurrentCoroutine;
	SetThis(this);
	DEBUG("before main func, use count %ld", GetThis().use_count());
	assert(m_eState != EXEC);
	m_eState = EXEC;
	SWAP_CONTEXT(pCurrentCoroutine->m_oCtx, m_oCtx);
}

// 切换到后台
void Coroutine::SwapOut(State eState/* = HOLD*/) {
	assert(m_pReturnCoroutine && m_eState == EXEC);
	DEBUG("cur use count %ld", GetThis().use_count());
	SetThis(m_pReturnCoroutine.get());
	m_eState = eState;
	volatile bool bFlag = true;
	GET_CONTEXT(m_oCtx);
	if (bFlag) {
		bFlag = false;
		m_oSpinlock.Unlock(); // 解锁之前要设置好 ctx，而要解锁之后才能切走
		SET_CONTEXT(m_pReturnCoroutine->m_oCtx);
	}
}

void Coroutine::Split() {
	// 由于 SwapOut 的设计，不到下面切出去它是不可能 SwapIn 的，
	// 并且下次 SwapIn 的时候其实就是到了这个 SwapOut 结束的地方。
	// 后面如果说一个任务执行一半需要阻塞就是把下面这个东西换成注
	// 册事件即可。
	cppsvr::ThreadPool::GetSingleton()->AddTask(GetThis());
	SwapOut();
}

uint64_t Coroutine::GetId() const {
	return m_iId;
}

Coroutine::State Coroutine::GetState() const {
	return m_eState;
}

void Coroutine::SetState(Coroutine::State eState) {
	m_eState = eState;
}

// 设置当前协程
void Coroutine::SetThis(Coroutine *pCurrentCoroutine) {
	t_pCurrentCoroutine = pCurrentCoroutine;
}

// 返回当前协程
Coroutine::ptr Coroutine::GetThis() {
	if (t_pCurrentCoroutine) {
		return t_pCurrentCoroutine->shared_from_this();
	}
	t_pMainCoroutine.reset(new Coroutine);
	t_pCurrentCoroutine = t_pMainCoroutine.get();
	return t_pCurrentCoroutine->shared_from_this();
}

// 总协程数
uint64_t Coroutine::TotalCoroutines() {
	return s_iCoroutineCount;
}

void Coroutine::MainFunc() {
	Coroutine::ptr pCurrentCoroutine = GetThis();
	assert(pCurrentCoroutine);
	DEBUG("begin main func, use count %ld", pCurrentCoroutine.use_count());
	try {
		pCurrentCoroutine->m_funCallBack();
	} catch (std::exception &ex) {
		pCurrentCoroutine->m_eState = EXCEPT;
		ERROR("coroutine except: %s. id %lu", ex.what(), pCurrentCoroutine->GetId());
	}
	pCurrentCoroutine->m_funCallBack = nullptr;
	Coroutine* pRow = pCurrentCoroutine.get();
	DEBUG("here555 use count %ld", pCurrentCoroutine.use_count());
	pCurrentCoroutine.reset(); // 否则影响销毁！
	pRow->SwapOut(TERM);
	// 上面一定会交换走，成功的话一定不会到下面这里。
	ERROR("coroutine should never come here. coroutine id %lu", pCurrentCoroutine->GetId());
}

}