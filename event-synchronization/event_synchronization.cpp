#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>

using namespace std::literals;

namespace EventsWithConditionVars
{
    class Data
    {
        std::vector<int> data_;
        bool is_ready_ = false;
        std::mutex mtx_is_ready_;
        std::condition_variable cv_data_ready_;

    public:
        void read()
        {
            std::osyncstream(std::cout) << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(2s);
            std::osyncstream(std::cout) << "End reading..." << std::endl;

            {
                std::lock_guard lk{mtx_is_ready_};
                is_ready_ = true;
            }
            cv_data_ready_.notify_all();
        }

        void process(int id)
        {
            std::unique_lock lk{mtx_is_ready_};
            cv_data_ready_.wait(lk, [&] { return is_ready_; });
            lk.unlock();

            long sum = std::accumulate(begin(data_), end(data_), 0L);
            std::osyncstream(std::cout) << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace EventsWithConditionVars

namespace ConditionVarsWithStopToken
{
    class Data
    {
        std::vector<int> data_;
        bool is_ready_ = false;
        std::mutex mtx_is_ready_;
        std::condition_variable_any cv_data_ready_;

    public:
        void read()
        {
            std::osyncstream(std::cout) << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(12s);
            std::osyncstream(std::cout) << "End reading..." << std::endl;

            {
                std::lock_guard lk{mtx_is_ready_};
                is_ready_ = true;
            }
            cv_data_ready_.notify_all();
        }

        void process(int id, std::stop_token stop_tkn)
        {
            std::unique_lock lk{mtx_is_ready_};
            bool was_cancelled = !cv_data_ready_.wait(lk, stop_tkn, [&] { return is_ready_; });
            if (was_cancelled)
            {
                std::osyncstream(std::cout) << "Processing has been cancelled...\n";
                return;
            }
            lk.unlock();

            long sum = std::accumulate(begin(data_), end(data_), 0L);
            std::osyncstream(std::cout) << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace ConditionVarsWithStopToken

class Data
{
    std::vector<int> data_;
    std::atomic<bool> is_ready_ = false;
    int temp;
    static_assert(std::atomic<double>::is_always_lock_free);

public:
    void read()
    {
        std::osyncstream(std::cout) << "Start reading..." << std::endl;
        data_.resize(100);

        std::random_device rnd;
        std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
        std::this_thread::sleep_for(2s);
        std::osyncstream(std::cout) << "End reading..." << std::endl;

        /////////////////////////////////////////////////////////
        // is_ready_ = true;
        is_ready_.store(true, std::memory_order_release);
    }

    void process(int id)
    {
        while (!is_ready_.load(std::memory_order_acquire))
        /////////////////////////////////////////////////////////
        {
            // std::this_thread::yield();
        }

        long sum = std::accumulate(begin(data_), end(data_), 0L);
        std::osyncstream(std::cout) << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

namespace Atomics
{
    class SpinLockMutex
    {
        std::atomic_flag flag_{};

    public:
        SpinLockMutex()
        { }

        void lock()
        {
            while (flag_.test_and_set(std::memory_order_acquire))
            {}
        }

        void unlock()
        {
            flag_.clear(std::memory_order_release);
        }
    };
} // namespace Atomics

int main()
{
    std::osyncstream(std::cout) << "Start of main..." << std::endl;
    {
        Data data;

        std::jthread thd_producer{[&data] { data.read(); }};

        std::stop_source stop_src;

        std::jthread thd_consumer_1{[&] { data.process(1); }};
        std::jthread thd_consumer_2{[&] { data.process(2); }};

        std::this_thread::sleep_for(3s);
        stop_src.request_stop();
    }

    std::osyncstream(std::cout) << "END of main..." << std::endl;
}
