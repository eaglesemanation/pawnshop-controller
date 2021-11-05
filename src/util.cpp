#include "pawnshop/util.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace pawnshop {

optional<string> getline_timeout(istream &is, duration<int> timeout) {
    timed_mutex mx;
    atomic<bool> stopReading = false;
    string line;
    line.reserve(128);

    auto t1 = thread([&]() {
        lock_guard<timed_mutex> lock(mx);
        while (!stopReading) {
            char c;
            if (is.readsome(&c, 1) > 0) {
                if (c != '\n') {
                    line.push_back(c);
                } else {
                    break;
                }
            }
        }
    });

    // Wait for t1 to lock mutex
    auto delay = 100ms;
    this_thread::sleep_for(delay);
    if (mx.try_lock_for(timeout - delay)) {
        t1.join();
        return line;
    } else {
        stopReading = true;
        t1.join();
        return {};
    }
}

}
