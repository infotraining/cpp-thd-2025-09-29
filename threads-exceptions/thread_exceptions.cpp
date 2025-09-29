#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
class Result
{
    T value_;
    std::exception_ptr exception_;
public:
    template <typename TValue>
    void set_value(TValue&& value)
    {
        value_ = std::forward<TValue>(value);
    }

    void set_exception(std::exception_ptr eptr)
    {
        exception_ = eptr;
    }

    T get()
    {
        if (exception_)
        {
            std::rethrow_exception(exception_);
        }

        return value_;
    }
};

void background_work(size_t id, const std::string& text, 
                     Result<char>& result)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(100ms);
        }

        result.set_value(text.at(5)); // potential exception

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
    catch(...)
    {
        result.set_exception(std::current_exception());
    }   
}

int main()
{
    std::cout << "No of cores: " << std::thread::hardware_concurrency() << std::endl;

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello";
    
    const size_t thread_count = 4;

    std::vector<std::string> arguments = {"Hello", "Concurrent", "Multithreading", ""};
    std::vector<std::jthread> threads(thread_count);
    std::vector<Result<char>> results(thread_count);

    for(size_t i = 0; i < threads.size(); ++i)
    {
        threads[i] = std::jthread{background_work, i, std::cref(arguments[i]), std::ref(results[i]) };
    }

    // explicit join
    for(auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    for(auto& r : results)
    {
        try
        {
            char result = r.get();

            std::cout << "Result: " << result << "\n";
        }
        catch(const std::exception& e)
        {
            std::cout << "Caught an exception: " << e.what() << "\n";
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
