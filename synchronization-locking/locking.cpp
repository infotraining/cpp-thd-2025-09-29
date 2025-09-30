#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

void run(int& value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        ++value;
    }
}

void may_throw()
{
    throw std::runtime_error("ERROR#13");
}

void run(int& value, std::mutex& mtx_value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        mtx_value.lock();
        std::lock_guard lk{mtx_value, std::adopt_lock}; // mtx_value.lock(); // start of critical section
        ++value;
        //may_throw();
    } // mtx_value.unlock(); // end of critical section
}

void run(std::atomic<int>& value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        ++value;
    }
}

constexpr uintmax_t factorial(uintmax_t n)
{
    if (n <= 1)
        return 1;
    return n * factorial(n-1);
}

template <size_t N>
constexpr auto create_factorial_lookup_table()
{
    std::array<uintmax_t, N> results{};

    for(size_t i = 0; i < N; ++i)
        results[i] = factorial(i);

    return results;
}

void test_lookup_table()
{
    //constexpr int test_r = hardening_code();
    constexpr auto lookup_factorial = create_factorial_lookup_table<13>();
}

void timed_mutex_demo()
{
    std::timed_mutex mutex;
    
    auto work_1 = [&](){
        std::cout << "START#1" << std::endl;
        std::unique_lock<std::timed_mutex> lock(mutex, std::try_to_lock);
        if (!lock.owns_lock())
        {
            do
            {
                std::cout << "Thread does not own a lock..."
                        << " Tries to acquire a mutex..."
                        << std::endl;
            } while(!lock.try_lock_for(std::chrono::seconds(1)));
        }

        std::cout << "Access#1 granted" << "\n";
    };

    auto work_2 = [&]() {
        std::cout << "START#2" << std::endl;
        std::unique_lock lk{mutex};
        std::this_thread::sleep_for(5s);
    };

    {
        std::jthread thd_2{work_2};
        std::this_thread::sleep_for(50ms);
        std::jthread thd_1{work_1};
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    // {
    //     auto start = std::chrono::high_resolution_clock::now();

    //     int counter = 0;

    //     {
    //         std::jthread thd_1{[&counter] { run(counter); }};
    //         std::jthread thd_2{[&counter] { run(counter); }};
    //     }

    //     auto end = std::chrono::high_resolution_clock::now();

    //     std::cout << "counter: " << counter << "\n";
    //     std::cout << "time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
    // }

    std::cout << "-------------\n";

    {
        auto start = std::chrono::high_resolution_clock::now();

        int counter = 0;
        std::mutex mtx_counter;

        {
            std::jthread thd_1{[&counter, &mtx_counter] { run(counter, mtx_counter); }};
            std::jthread thd_2{[&counter, &mtx_counter] { run(counter, mtx_counter); }};
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "counter: " << counter << "\n";
        std::cout << "time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
    }

    std::cout << "-------------\n";

    {
        auto start = std::chrono::high_resolution_clock::now();

        std::atomic<int> counter = 0;
        {
            std::jthread thd_1{[&counter] { run(counter); }};
            std::jthread thd_2{[&counter] { run(counter); }};
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "counter: " << counter << "\n";
        std::cout << "time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start) << "\n";
    }

    std::cout << "Main thread ends..." << std::endl;

    timed_mutex_demo();
}

