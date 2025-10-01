#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <syncstream>
#include <future>

std::osyncstream sync_cout()
{
    return std::osyncstream{std::cout};
}

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

int calculate_square(int x)
{
    sync_cout() << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
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
    using Task = std::move_only_function<void()>; // since C++23

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
                tasks_.push([this] { this->end_of_work_ = true; });
            }

            for (auto& thd : threads_)
            {
                if (thd.joinable())
                    thd.join();
            }
        }

        template <typename FunctionTask>        
        auto submit(FunctionTask&& ftask)
        {
            using TResult = decltype(ftask());
            std::packaged_task<TResult()> pt{std::move(ftask)};
            std::future<TResult> f_result = pt.get_future();

            auto task = [pt = std::move(pt)]() mutable { pt(); };
            tasks_.push(std::move(task));

            return f_result;
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

        ///////////////////////////////////////////////////////////////////////////

        std::future<int> fs_13 = thd_pool.submit([] { return calculate_square(13); });
        sync_cout() << "Square for 13: " << fs_13.get() << "\n";

        std::vector<std::tuple<int, std::future<int>>> f_sqaures;
        for(int i = 10; i < 30; ++i)
        {
            f_sqaures.emplace_back(i, thd_pool.submit([i] { return calculate_square(i); }));
        }

        for(auto& [n, f_sqaure] : f_sqaures)
        {
            try
            {
                int square = f_sqaure.get();
                sync_cout() << n << " * " << n << " = " << square << "\n";
            }
            catch(const std::exception& e)
            {
                std::cerr << "Exception for " << n << ": " << e.what() << '\n';
            }            
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
