#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <syncstream>

using namespace std::literals;

class Data
{
    std::vector<int> data_;
    bool is_ready_ = false;
    std::mutex mtx_is_ready_;

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
    }

    void process(int id)
    {
        auto data_ready = [&]() { 
            std::lock_guard lk{mtx_is_ready_};
            return is_ready_;
        };

        while(!data_ready())
        {}

        long sum = std::accumulate(begin(data_), end(data_), 0L);
        std::osyncstream(std::cout) << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

int main()
{
    std::osyncstream(std::cout) << "Start of main..." << std::endl;
    {
        Data data;
        std::jthread thd_producer{[&data] { data.read(); }};

        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
    }

    std::osyncstream(std::cout) << "END of main..." << std::endl;
}
