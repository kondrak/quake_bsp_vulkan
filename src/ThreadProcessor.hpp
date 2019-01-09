#ifndef THREADPROCESSOR_HPP
#define THREADPROCESSOR_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

typedef std::function<void()> ThreadTask;

class ThreadProcessor
{
public:
    ~ThreadProcessor();

    const unsigned int NumThreads() const { return m_numThreads; }
    void SpawnWorkers();
    void AddTask(uint8_t threadId, ThreadTask &&task);
    void Wait();
    void Finish();
private:
    // worker is a set of tasks executed per-thread
    struct Worker
    {
        Worker();
        ~Worker();
        void AddTask(ThreadTask &&task);
        void Work();
        void WaitComplete();
        void Finish();
        std::queue<ThreadTask> tasks;
        std::mutex taskMutex;
        std::thread thread;
        std::condition_variable cv;
        bool finish = false;
    };

    unsigned int m_numThreads = 1;
    std::vector<Worker> m_workers;
};

#endif
