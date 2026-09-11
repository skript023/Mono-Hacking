#include "class/triple_buffer.hpp"
#include <atomic>
#include <cassert>
#include <string>
#include <thread>

struct Entry
{
    static inline std::atomic<int> alive = 0;
    int generation;
    std::string name;
    explicit Entry(int value) : generation(value), name(256, char('a' + value % 26)) { ++alive; }
    Entry(const Entry&) = delete; // Reading must work without copying entries.
    Entry(Entry&& other) noexcept : generation(other.generation), name(std::move(other.name)) { ++alive; }
    ~Entry() { --alive; }
};

int main()
{
    big::TripleBuffer<Entry> buffer(64);
    std::atomic<bool> done = false;
    std::atomic<int> reads = 0;
    std::thread reader([&] {
        do
        {
            const auto view = buffer.view();
            if (!view.empty())
            {
                const int generation = view.begin()->generation;
                for (const auto& entry : view)
                {
                    assert(entry.generation == generation);
                    assert(entry.name == std::string(256, char('a' + generation % 26)));
                    std::this_thread::yield(); // Let publication race a deliberately slow reader.
                }
                ++reads;
            }
        } while (!done.load());
    });
    for (int generation = 0; generation < 20000; ++generation)
    {
        auto& back = buffer.back();
        back.clear();
        for (int i = 0; i < 16; ++i) back.emplace_back(generation);
        buffer.publish();
        if (generation == 0)
            while (reads.load() == 0) std::this_thread::yield();
        if (generation % 100 == 0) buffer.clear_all();
    }
    done = true;
    reader.join();
    assert(reads > 0);
    buffer.clear_all();
    assert(Entry::alive == 0);
    assert(buffer.back().capacity() == 0);
    assert(buffer.view().empty());
    buffer.back().emplace_back(42);
    buffer.publish();
    {
        const auto view = buffer.view();
        assert(view.begin()->generation == 42);
    }
    buffer.clear_all();
    assert(Entry::alive == 0);
}
