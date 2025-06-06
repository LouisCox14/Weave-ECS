#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace Weave
{
    namespace Utilities
    {
        class ThreadPool
        {
        private:
            std::vector<std::thread> workers;
            std::queue<std::function<void()>> tasks;
            std::mutex queueMutex;
            std::condition_variable condition;
            bool stop = false;
            std::atomic<int> activeTasks{ 0 };
            std::condition_variable completionCondition;

        public:
            explicit ThreadPool(size_t numThreads)
            {
                for (size_t i = 0; i < numThreads; ++i)
                {
                    workers.emplace_back([this]() {
                        while (true)
                        {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(queueMutex);
                                condition.wait(lock, [this] { return stop || !tasks.empty(); });

                                if (stop && tasks.empty()) return;

                                task = std::move(tasks.front());
                                tasks.pop();
                                activeTasks++;
                            }

                            task();

                            {
                                std::lock_guard<std::mutex> lock(queueMutex);
                                activeTasks--;
                                if (activeTasks == 0 && tasks.empty()) {
                                    completionCondition.notify_all();
                                }
                            }
                        }});
                }
            }

            template <typename F, typename... Args>
            auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
            {
                using ReturnType = typename std::invoke_result<F, Args...>::type;
                auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

                std::future<ReturnType> result = task->get_future();
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (stop) throw std::runtime_error("ThreadPool has stopped accepting tasks.");
                    tasks.emplace([task]() { (*task)(); });
                }
                condition.notify_one();
                return result;
            }

            size_t GetThreadCount()
            {
                return workers.size();
            }

            void WaitAll()
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                completionCondition.wait(lock, [this]() {
                    return tasks.empty() && activeTasks == 0;
                    });
            }

            ~ThreadPool()
            {
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    stop = true;
                }
                condition.notify_all();
                for (std::thread& worker : workers) {
                    if (worker.joinable()) worker.join();
                }
            }
        };
    }
}