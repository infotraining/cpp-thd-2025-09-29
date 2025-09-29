#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

void background_work(size_t id, const std::string& text, 
                     char& result, std::exception_ptr& eptr)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(100ms);
        }

        result = text.at(5); // potential exception

        std::cout << "bw#" << id << " is finished..." << std::endl;
    }
    catch(...)
    {
        eptr = std::current_exception();
    }   
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello";
    
    char result = 0;
    std::exception_ptr eptr;

    {
        std::jthread thd_1{background_work, 1, std::cref(text), 
                           std::ref(result), std::ref(eptr) };
    }

    if (eptr)
    {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch(const std::out_of_range& e)
        {
            std::cout << "Caught an exception: " << e.what() << "\n";
        }
        catch(const std::exception& e)
        {
            std::cout << "Std exception caught: " << e.what() << "\n";
        }
    }
    else
    {
        std::cout << "Result: " << result << "\n";
    }

    std::cout << "Main thread ends..." << std::endl;
}
