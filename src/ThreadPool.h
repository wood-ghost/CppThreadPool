#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <future>
#include <any>
#include <utility>

class ThreadPool {
  public:
    class Task {
      public:
        std::promise<std::any> p;
        std::function<void(std::promise<std::any>)> func;
        void execute() {
            auto token = std::move(p);
            func(std::move(token));
        }
        Task(std::promise<std::any> _p, std::function<void(std::promise<std::any>)> _func) : p(std::move(_p)), func(std::move(_func)) {}
        Task(const Task &) = delete;
        Task (Task && t) : p(std::move(t.p)), func(std::move(t.func)){}
    };
    using TaskPromise = std::promise<std::any>;
    using TaskFunc = std::function<void(std::promise<std::any>)>; 
    

    ThreadPool(const ThreadPool &) = delete;

    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;

    static ThreadPool& getInstance(); 

    ~ThreadPool();

    // return the number of worker threads
    size_t size() const;

    // submit a task to the thread pool
    void submitTask(Task&& task);

    // shutdown the thread pool
    void shutdown();

    // block the calling thread and waits for the completion of all tasks.
    void waitTasks();

    // pop a task from queue and blocking the calling thread if the queue is
    // empty.
    Task popTask();

    // add new worker
    void addWorker();

  private:
    std::atomic<bool> is_active = {
        true}; // Flag indicating that the thread pool is active
    std::queue<Task> task_queue;     // Task queue
    std::deque<std::thread> workers; // Worker threads
    std::mutex qmutex;               // Mutex for protecting the queue
    std::condition_variable qtask;   // conditional var for tasks in queue
    std::condition_variable qempty;  // conditional var for empty task queue

    // create a thread pool with N worker threads
    ThreadPool(size_t n);

    // the thread pool will use MAX-1 cores of the computer by default
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency() - 1) {}
};
