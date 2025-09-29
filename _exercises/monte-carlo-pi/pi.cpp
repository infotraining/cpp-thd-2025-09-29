#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <future>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <atomic>

/*******************************************************
 * https://academo.org/demos/estimating-pi-monte-carlo
 *******************************************************/

using namespace std;

void calc_hits(const uintmax_t count, uintmax_t& hits)
{
    const auto seed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::mt19937_64 rnd_gen(seed);
    std::uniform_real_distribution<double> rnd_distr(0.0, 1.0);

    uintmax_t local_hits = 0;
    for (long n = 0; n < count; ++n) // hot-loop
    {
        double x = rnd_distr(rnd_gen);
        double y = rnd_distr(rnd_gen);
        if (x * x + y * y < 1)
            local_hits++;
    }
    hits = local_hits;
}

void calc_hits_cache_ping_pong(const uintmax_t count, uintmax_t& hits)
{
    const auto seed = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::mt19937_64 rnd_gen(seed);
    std::uniform_real_distribution<double> rnd_distr(0.0, 1.0);
    for (long n = 0; n < count; ++n) // hot-loop
    {
        double x = rnd_distr(rnd_gen);
        double y = rnd_distr(rnd_gen);
        if (x * x + y * y < 1)
            hits++;
    }
}

double single_thread_pi(uintmax_t count)
{
    uintmax_t hits{};

    calc_hits(count, hits);

    const double pi = static_cast<double>(hits) / count * 4;

    return pi;
}

double multi_thread_pi(const uintmax_t count, const size_t no_of_cores = std::thread::hardware_concurrency())
{
    const uintmax_t count_per_thread = count / no_of_cores;

    std::vector<uintmax_t> partial_hits(no_of_cores);
    {
        std::vector<std::jthread> threads(no_of_cores);

        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i] = std::jthread{calc_hits, count_per_thread, std::ref(partial_hits[i])};
        }
    } // implicit join

    const uintmax_t hits = std::accumulate(partial_hits.begin(), partial_hits.end(), std::uintmax_t{});

    const double pi = static_cast<double>(hits) / count * 4;

    return pi;
}

double multi_thread_pi_cache_ping_pong(const uintmax_t count, const size_t no_of_cores = std::thread::hardware_concurrency())
{
    const uintmax_t count_per_thread = count / no_of_cores;

    std::vector<uintmax_t> partial_hits(no_of_cores);
    {
        std::vector<std::jthread> threads(no_of_cores);

        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i] = std::jthread{calc_hits_cache_ping_pong, count_per_thread, std::ref(partial_hits[i])};
        }
    } // implicit join

    const uintmax_t hits = std::accumulate(partial_hits.begin(), partial_hits.end(), std::uintmax_t{});

    const double pi = static_cast<double>(hits) / count * 4;

    return pi;
}

int main()
{
    const size_t no_of_threads = std::thread::hardware_concurrency();
    std::cout << "No of hardware threads: " << no_of_threads << "\n";
    std::cout << "Cache Line Size: " << std::hardware_destructive_interference_size << "\n";

    const uintmax_t N = 1'000'000'000;

    //////////////////////////////////////////////////////////////////////////////
    // single thread
    {
        cout << "Single thread - Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        const double pi = single_thread_pi(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////
    // multithreading - cache ping-pong
    {
        cout << "Multithreading Cache Ping-Pong - Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        const double pi = multi_thread_pi_cache_ping_pong(N, no_of_threads);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////
    // multithreading
    {
        cout << "Multithreading - Pi calculation started!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        const double pi = multi_thread_pi(N, no_of_threads);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }
}
