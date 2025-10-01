#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <syncstream>

using namespace std::literals;

std::osyncstream sync_cout()
{
    return std::osyncstream{std::cout};
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

void save_to_file(const std::string& filename)
{
    sync_cout() << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    sync_cout() << "File saved: " << filename << std::endl;
}

void consumer(int id, std::shared_future<int> fs)
{
    sync_cout() << "Consumer#" << id << "\n";

    try
    {
        int result = fs.get(); 
        sync_cout() << "Result: " << result << "\n";
    }
    catch(const std::exception& e)
    {
        sync_cout() << "Exception: " << e.what() << '\n';
    }
}

class SquareCalulator
{
    std::promise<int> promise_result_;
public:
    std::future<int> get_future()
    {
        return promise_result_.get_future();
    }

    void calculate(int n)
    {
        try
        {
            int n_square = calculate_square(n);
            promise_result_.set_value(n_square);
        }
        catch(...)
        {
            promise_result_.set_exception(std::current_exception());
        }
    }
};

int main()
{
    sync_cout() << "Main thread starts...\n";

    std::future<int> f_square_13 = std::async(std::launch::async, calculate_square, 13);   
    std::future<int> f_square_9 = std::async(calculate_square, 9);
    std::future<void> f_save = std::async([] { save_to_file("data.txt"); });   

    while (f_save.wait_for(100ms) != std::future_status::ready)
    {
        sync_cout() << ".";
        std::cout.flush();
    }

    try
    {
        int square_13 = f_square_13.get();
        sync_cout() << "Result for 13: " << square_13 << "\n";

        int square_9 = f_square_9.get();
    }
    catch(const std::exception& e)
    {
        sync_cout() << "Caught an exception: " << e.what() << "\n";
    }

    sync_cout() << "\n-------------------------------\n";

    std::vector<std::tuple<int, std::future<int>>> f_squares;

    for(const int n : {7, 13, 77, 101, 99, 44, 42})
    {
        f_squares.emplace_back(n, std::async(std::launch::async, calculate_square, n));
    }

    for(auto& [n, fs] : f_squares)
    {
        try
        {
            int result = fs.get();
            sync_cout() << "Result for " << n << ": " << result << "\n";
        }
        catch(const std::exception& e)
        {
            sync_cout() << "Exception for " << n << ": " << e.what() << "\n";
        }
    }

    /////////////////////////////////////////////////////////
    // std::future vs. std::shared_future

    std::future<int> f_square_144 = std::async(std::launch::async, calculate_square, 144);

    std::shared_future<int> shared_square_144 = f_square_144.share();

    {
        std::jthread thd_1{consumer, 1, shared_square_144};
        std::jthread thd_2{consumer, 2, shared_square_144};
        std::jthread thd_3{consumer, 3, shared_square_144};
    }

    sync_cout() << "END of MAIN\n";

    ////////////////////////////////////////////////////////
    // std::promise

    SquareCalulator calc;
    std::future<int> f_calc = calc.get_future();

    {
        std::jthread thd_1{[&calc] { calc.calculate(13); }};
        std::jthread thd_2{[f = std::move(f_calc)]() mutable { 
            sync_cout() << "Result from SquareCalculator: " << f.get() << "\n";
        }};
    }
}
