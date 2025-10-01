#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

namespace ver_1
{
    using Task = std::function<void()>;

    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t thread_count)
            : threads_(thread_count)
        {
            for (auto& thd : threads_)
                thd = std::thread{[this] { run(); }};
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
            {
                // sending poisoning pill
                tasks_.push(Task{});
            }

            for (auto& thd : threads_)
            {
                if (thd.joinable())
                    thd.join();
            }
        }

        void submit(Task task)
        {
            assert(task != nullptr && "Task cannot be empty");

            tasks_.push(task);
        }

    private:
        ThreadSafeQueue<Task> tasks_;
        std::vector<std::thread> threads_;

        void run()
        {
            while (true)
            {
                Task task;
                tasks_.pop(task);

                if (is_end_of_work(task))
                    return;

                task(); // execution of task
            }
        }

        bool is_end_of_work(Task task)
        {
            return task == nullptr;
        }
    };
} // namespace ver_1

inline namespace ver_2
{
    using Task = std::function<void()>;

    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t thread_count)
            : threads_(thread_count), end_of_work_{false}
        {
            for (auto& thd : threads_)
                thd = std::thread{[this] { run(); }};
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
            {
                tasks_.push([this] { end_of_work_ = true; });
            }

            for (auto& thd : threads_)
            {
                if (thd.joinable())
                    thd.join();
            }
        }

        void submit(Task task)
        {
            assert(task != nullptr && "Task cannot be empty");

            tasks_.push(task);
        }

    private:
        ThreadSafeQueue<Task> tasks_;
        std::vector<std::thread> threads_;
        std::atomic<bool> end_of_work_;

        void run()
        {
            while (!end_of_work_)
            {
                Task task;
                tasks_.pop(task);

                task(); // execution of task
            }
        }
    };
} // namespace ver_1

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        const size_t no_of_cores = std::thread::hardware_concurrency();

        ThreadPool thd_pool(no_of_cores);

        thd_pool.submit([] { background_work(1, "Text", 25ms); });
        thd_pool.submit([] { background_work(2, "Hello", 75ms); });
        thd_pool.submit([] { background_work(3, "ThreadPool", 125ms); });

        for (int i = 4; i < 30; ++i)
            thd_pool.submit([=] { background_work(i, "TASK#" + std::to_string(i), 100ms); });
    }

    std::cout << "Main thread ends..." << std::endl;
}
