#include <ucontext.h>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

struct Task {
    ucontext_t context;
    bool finished = false;
};

std::queue<Task*> taskQueue;
std::mutex queueMutex;
std::condition_variable queueCV;
bool stopPool = false;

void task_function(Task* task) {
    std::cout << "Task started in thread: " << std::this_thread::get_id() << std::endl;

    // Save current context and switch to another task (simulating task split)
    getcontext(&task->context);
    if (!task->finished) {
        task->finished = true;
        // Add this task back to the queue for continuation
        std::unique_lock<std::mutex> lock(queueMutex);
        taskQueue.push(task);
        queueCV.notify_one();
        return;
    }

    std::cout << "Task continued in thread: " << std::this_thread::get_id() << std::endl;
}

void worker() {
    while (true) {
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [] { return !taskQueue.empty() || stopPool; });

            if (stopPool && taskQueue.empty()) {
                return;
            }

            task = taskQueue.front();
            taskQueue.pop();
        }

        setcontext(&task->context);
    }
}

int main() {
    // Create thread pool
    const int numThreads = 4;
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }

    // Create and initialize task
    Task task;
    char stack[8192];
    getcontext(&task.context);
    task.context.uc_stack.ss_sp = stack;
    task.context.uc_stack.ss_size = sizeof(stack);
    task.context.uc_link = nullptr;
    makecontext(&task.context, (void (*)())task_function, 1, &task);

    // Add initial task to the queue
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        taskQueue.push(&task);
    }
    queueCV.notify_one();

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "All tasks completed" << std::endl;
    return 0;
}
