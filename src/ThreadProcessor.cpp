#include "ThreadProcessor.hpp"
#include "Utils.hpp"

ThreadProcessor::Worker::Worker()
{
    thread = std::thread(&Worker::Work, this);
}

ThreadProcessor::Worker::~Worker()
{
    Finish();
}

void ThreadProcessor::Worker::AddTask(ThreadTask &&task)
{
    std::unique_lock<std::mutex> lock(taskMutex);
    tasks.emplace(task);
    cv.notify_one();
}

void ThreadProcessor::Worker::Work()
{
    while (true)
    {
        std::unique_lock<std::mutex> queueLock(taskMutex);
        cv.wait(queueLock, [this]() { return !tasks.empty() || finish; });
        if(finish)
            break;

        // execute
        tasks.front()();

        // remove from task list
        tasks.pop();
        cv.notify_one();
    }
}

void ThreadProcessor::Worker::WaitComplete()
{
    std::unique_lock<std::mutex> queueLock(taskMutex);
    cv.wait(queueLock, [this]() { return tasks.empty(); });
}

void ThreadProcessor::Worker::Finish()
{
    if (thread.joinable())
    {
        WaitComplete();
        taskMutex.lock();
        finish = true;
        cv.notify_one();
        taskMutex.unlock();
        thread.join();
    }
}

ThreadProcessor::~ThreadProcessor()
{
    Finish();
}

void ThreadProcessor::Init()
{
    m_numThreads = std::thread::hardware_concurrency();
    LOG_MESSAGE("Found " << m_numThreads << " threads.");
    m_workers = std::vector<Worker>(m_numThreads);
}

void ThreadProcessor::AddTask(uint8_t threadIdx, ThreadTask &&task)
{
    m_workers[threadIdx].AddTask(std::move(task));
}

void ThreadProcessor::Wait()
{
    for (auto &worker : m_workers)
    {
        worker.WaitComplete();
    }
}

void ThreadProcessor::Finish()
{
    for (auto &worker : m_workers)
    {
        worker.Finish();
    }
}
