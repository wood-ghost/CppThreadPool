#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t n) {
    std::cerr << "<Constructor> Construct a Thread Pool with " << n
              << " worker threads" << std::endl;
    for (size_t i = 0; i < n; ++i) {
        this->addWorker();
    }
}

ThreadPool::~ThreadPool() {
    std::cerr << "<Destructor> Waiting for workers to shutdown." << std::endl;
    this->shutdown();
    std::cerr << "<Destructor> All Worker threads have shutdown." << std::endl;
}

ThreadPool & ThreadPool::getInstance() { 
    static std::unique_ptr<ThreadPool> instance;
        if (instance == nullptr) {
            instance = std::unique_ptr<ThreadPool>(new ThreadPool);
        }
        return *instance;
    }

size_t ThreadPool::size() const { return this->workers.size(); }

void ThreadPool::submitTask(ThreadPool::Task&& task) {
    // set a guard for queue
    std::lock_guard<std::mutex> guard(qmutex);
    // push task to task queue
    task_queue.emplace(std::move(task));
    // wake up a worker thread
    qtask.notify_one();
}

void ThreadPool::shutdown() {
    // disable the thread pool form submitting tasks
    is_active = false;
    // wake up all workers
    qtask.notify_all();
    // wait for all workers to complete
    for (auto &worker : workers) {
        worker.join();
    }
    // remove workers
    workers.clear();
}

void ThreadPool::waitTasks() {
    std::unique_lock<std::mutex> lock(qmutex);
    // wait until all tasks are finished
    qempty.wait(lock, [&] { return task_queue.empty(); });
    // lock.unlock();
}

ThreadPool::Task ThreadPool::popTask() {
    std::unique_lock<std::mutex> lock(qmutex);
    // wait until the task queue is not empty
    qtask.wait(lock, [&] { return !task_queue.empty() || (!is_active); });
    if (!task_queue.empty()) {
        // get the first task
        auto task = std::move(task_queue.front());
        task_queue.pop();
        // wake up all workers when the all the task queue is empty
        if (task_queue.empty()) {
            qempty.notify_all();
        }
        return task;
    }
    return Task(TaskPromise(), TaskFunc());
}

void ThreadPool::addWorker() {
    auto th = std::thread([this] {
        while (true) {
            auto task = popTask();
            if (task.func != nullptr) {
                task.execute();
            }
            std::lock_guard<std::mutex> guard(qmutex);
            if (!is_active && task_queue.empty()) {
                return;
            }
        }
    });
    std::cerr << "<addWorker> new worker thread ID = " << th.get_id()
              << std::endl;
    workers.push_back(std::move(th));
}
