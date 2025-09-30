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
        std::condition_variable cv_data_ready;

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
            cv_data_ready.notify_all();
        }

        void process(int id)
        {
            std::unique_lock lk{mtx_is_ready_};
            cv_data_ready.wait(lk, [&] { return is_ready_; });
            lk.unlock();

            long sum = std::accumulate(begin(data_), end(data_), 0L);
            std::osyncstream(std::cout) << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };
} // namespace EventsWithConditionVars

int main()
{
    using EventsWithConditionVars::Data;

    std::osyncstream(std::cout) << "Start of main..." << std::endl;
    {
        Data data;

        std::jthread thd_producer{[&data] { data.read(); }};

        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
    }

    std::osyncstream(std::cout) << "END of main..." << std::endl;
}
