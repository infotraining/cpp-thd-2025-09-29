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

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay = 250ms) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

void show_id()
{
    std::cout << "Hello from THD#" << std::this_thread::get_id() << std::endl;
}

void print(auto vec, std::string_view name)
{
    std::cout << name << ": ";
    for(const auto& item : vec)
        std::cout << item << " ";
    std::cout << std::endl;
}

void is_thread_safe()
{
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    std::vector<int> target_1;
    std::vector<int> target_2;

    // copy data to targets
    std::thread thd_copy_1([&data, &target_1] { target_1 = data; });
    std::thread thd_copy_2([&data, &target_2] { target_2 = data; });

    thd_copy_1.join();
    print(target_1, "target_1");
    thd_copy_2.join();
    print(target_2, "target_2");
}

void thread_demo()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    is_thread_safe();
    
    show_id();

    std::thread empty_thd;
    std::cout << "empty_thd: " << empty_thd.get_id() << "\n";

    std::vector<std::thread> threads;    

    std::thread thd_1{background_work, 1, std::cref(text), 150ms};
    std::thread thd_2{BackgroundWork{2, "Concurrent"}, 207ms };

    threads.push_back(std::move(thd_1));
    threads.push_back(std::move(thd_2));

    BackgroundWork bw_3{3, "Multithreading"};
    std::thread thd_3{std::ref(bw_3), 100ms};

    threads.push_back(std::move(thd_3));
    threads.push_back(std::thread{[] { background_work(4, "ConcurrentLambda", 153ms);}});

    BackgroundWork bw_5{5, "MULTITHREADING"};
    std::thread thd_5{[&bw_5] { bw_5(600ms); }};
        
    thd_5.detach();
    assert(thd_5.joinable() == false);

    // joining for all threads
    for(auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    std::cout << "Main thread ends..." << std::endl;
}

void cancellable_background_work(std::stop_token stop_tkn, int id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "BW#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            if (stop_tkn.stop_requested())
            {
                std::cout << "Stop requested..." << std::endl;
                break;
            }

            std::cout << "BW#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id << " is finished..." << std::endl;
}

std::jthread create_thread(std::stop_token stop_tkn, std::chrono::milliseconds delay)
{
    static int id = 1000;
    return std::jthread(cancellable_background_work, stop_tkn, ++id, "JTHREAD", delay);
}

void jthread_demo()
{
    std::stop_source stop_src;
    std::stop_token stop_tkn = stop_src.get_token();

    std::jthread thd_1{cancellable_background_work, stop_tkn, 1, "Hello", 42ms};
    std::jthread thd_2 = create_thread(stop_src.get_token(), 64ms);

    std::vector<std::jthread> jthreads;
    for(int i = 3; i < 10; ++i)
        jthreads.push_back(create_thread(stop_src.get_token(), 250ms));

    std::this_thread::sleep_for(1s);
    stop_src.request_stop();
} // joining all jthreads


thread_local std::string request_id;

void log(const std::string& message) {
    std::cout << "[Request " << request_id << "] " << message << "\n";
}

void handle_request(const std::string& id) 
{
    request_id = id;  // Set thread-local context
    log("Processing started");
    // ... more work ...
    log("Processing finished");
}

void thread_local_storage_demo() 
{
    std::thread t1(handle_request, "abc123");
    std::thread t2(handle_request, "xyz789");

    t1.join();
    t2.join();
}


int main()
{
    const size_t no_of_cores = std::max(1u, std::thread::hardware_concurrency());
    std::cout << "No of cores: " << no_of_cores << std::endl;

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";
    
    jthread_demo();

    std::cout << "End of main..." << std::endl;

    thread_local_storage_demo();
}
