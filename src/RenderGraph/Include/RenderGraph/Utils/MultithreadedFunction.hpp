#ifndef MULTITHREADED_FUNCTION_HPP
#define MULTITHREADED_FUNCTION_HPP

#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

class MultithreadedFunction final {
public:
    using FunctionType = std::function<void (uint32_t threadCount, uint32_t threadIndex)>;

private:
    bool ended;

    std::vector<std::thread> threads;

public:
    MultithreadedFunction (const uint32_t threadCount, const FunctionType& threadFunc)
        : ended (false)
    {
        threads.reserve (threadCount);
        for (uint32_t threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            threads.emplace_back (threadFunc, threadCount, threadIndex);
        }
    }

    MultithreadedFunction (const FunctionType& threadFunc)
        : MultithreadedFunction (std::thread::hardware_concurrency (), threadFunc)
    {
    }

    void Wait ()
    {
        if (ended) {
            return;
        }

        for (auto& t : threads) {
            t.join ();
        }

        ended = true;
    }

    ~MultithreadedFunction ()
    {
        Wait ();
    }
};

#endif