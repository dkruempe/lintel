#include <iostream>
#include <chrono>
#include <date/date.h>

int main(int argc, char *argv[]) {
    std::cout << "Hello World\n";
    const auto tp = std::chrono::system_clock::now();
    std::cout << date::format("%Y-%m-%d %H:%M:%S%Ez", tp) << "\n";
    auto ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto micsDuration = std::chrono::duration_cast<std::chrono::microseconds>(tp - std::chrono::system_clock::from_time_t(ts));
    std::cout << ts << "\n";
    auto tpCpy = std::chrono::system_clock::from_time_t(ts);
    std::cout << date::format("%Y-%m-%d %H:%M:%S%Ez", tpCpy) << "\n";
    std::cout << micsDuration.count() << "mics\n";
    auto mics = static_cast<int64_t>(micsDuration.count());
    const std::chrono::microseconds micsCpy(mics);
    tpCpy = tpCpy + micsCpy;
    std::cout << date::format("%Y-%m-%d %H:%M:%S%Ez", tpCpy) << "\n";
    return 0;
}