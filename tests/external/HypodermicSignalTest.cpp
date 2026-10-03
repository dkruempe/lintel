#include "Hypodermic/Signal.h"

#include <catch2/catch_all.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/**
 * Hypodermic::Signal replaced boost::signals2::signal in the vendored DI
 * container (see external/Hypodermic/NOTICE). These tests pin the semantics
 * the container depends on, most importantly the re-entrant case:
 * ContainerBuilder::listenToRegistrationDescriptorUpdates() calls
 * disconnect_all_slots() from inside a running slot.
 */

TEST_CASE("HypodermicSignal: emits to a connected slot") {
    Hypodermic::Signal<int> signal;

    int received = 0;
    signal.connect([&received](int value) { received = value; });

    signal(42);
    REQUIRE(received == 42);
}

TEST_CASE("HypodermicSignal: emits to slots in connection order") {
    Hypodermic::Signal<int> signal;

    std::vector<int> order;
    signal.connect([&order](int) { order.push_back(1); });
    signal.connect([&order](int) { order.push_back(2); });
    signal.connect([&order](int) { order.push_back(3); });

    signal(0);
    REQUIRE(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("HypodermicSignal: emitting without slots is a no-op") {
    Hypodermic::Signal<int> signal;
    REQUIRE_NOTHROW(signal(1));
}

TEST_CASE("HypodermicSignal: empty slots are ignored") {
    Hypodermic::Signal<int> signal;

    int calls = 0;
    signal.connect(Hypodermic::Signal<int>::Slot());
    signal.connect([&calls](int) { ++calls; });

    signal(0);
    REQUIRE(calls == 1);
}

TEST_CASE("HypodermicSignal: disconnect_all_slots stops delivery") {
    Hypodermic::Signal<int> signal;

    int calls = 0;
    signal.connect([&calls](int) { ++calls; });

    signal(0);
    signal.disconnect_all_slots();
    signal(0);

    REQUIRE(calls == 1);
}

TEST_CASE("HypodermicSignal: slot may disconnect_all_slots while emitting") {
    Hypodermic::Signal<int> signal;

    int firstCalls = 0;
    int secondCalls = 0;
    signal.connect([&firstCalls](int) { ++firstCalls; });
    signal.connect([&signal, &secondCalls](int) {
        ++secondCalls;
        // the pattern ContainerBuilder uses to drop its own slot
        signal.disconnect_all_slots();
    });
    signal.connect([](int) { FAIL("slot connected after the disconnect must not run"); });

    // The disconnecting slot runs second, so the first slot already saw this
    // emission while the third one must not - and iterating the live slot list
    // instead of a copy would be undefined behaviour here.
    REQUIRE_NOTHROW(signal(0));

    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 1);

    // and the signal is inert afterwards
    signal(0);
    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 1);
}

TEST_CASE("HypodermicSignal: slot may connect while emitting") {
    Hypodermic::Signal<> signal;

    int lateCalls = 0;
    signal.connect([&signal, &lateCalls] { signal.connect([&lateCalls] { ++lateCalls; }); });

    REQUIRE_NOTHROW(signal());
    REQUIRE(lateCalls == 0);

    // the slot added during emission runs on the next emission
    signal();
    REQUIRE(lateCalls == 1);
}

TEST_CASE("HypodermicSignal: Connection::disconnect detaches only its own slot") {
    Hypodermic::Signal<int> signal;

    int firstCalls = 0;
    int secondCalls = 0;
    auto first = signal.connect([&firstCalls](int) { ++firstCalls; });
    signal.connect([&secondCalls](int) { ++secondCalls; });

    signal(0);
    first.disconnect();
    signal(0);

    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 2);
}

TEST_CASE("HypodermicSignal: Connection::disconnect is idempotent") {
    Hypodermic::Signal<int> signal;

    int calls = 0;
    auto connection = signal.connect([&calls](int) { ++calls; });

    connection.disconnect();
    REQUIRE_NOTHROW(connection.disconnect());

    signal(0);
    REQUIRE(calls == 0);
}

TEST_CASE("HypodermicSignal: default constructed Connection is inert") {
    Hypodermic::Signal<int>::Connection connection;
    REQUIRE_NOTHROW(connection.disconnect());
}

TEST_CASE("HypodermicSignal: forwards multiple arguments by reference") {
    Hypodermic::Signal<std::string &, const std::shared_ptr<int> &> signal;

    std::string argument;
    int observed = 0;
    auto payload = std::make_shared<int>(7);
    signal.connect([&observed](std::string &value, const std::shared_ptr<int> &shared) {
        value = "seen";
        observed = *shared;
    });

    signal(argument, payload);

    REQUIRE(argument == "seen");
    REQUIRE(observed == 7);
}

TEST_CASE("HypodermicSignal: emits while another thread holds the slot list") {
    // The DI container can resolve from several threads, so connect/emit must
    // not corrupt the slot list. Not a race detector, just a smoke test that
    // the mutex-guarded copy keeps the bookkeeping consistent.
    Hypodermic::Signal<int> signal;

    std::atomic_int calls{0};
    for (int i = 0; i < 64; ++i) {
        signal.connect([&calls](int) { ++calls; });
    }

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&signal] {
            for (int i = 0; i < 250; ++i) signal(i);
        });
    }
    for (auto &thread: threads) thread.join();

    REQUIRE(calls.load() == 64 * 4 * 250);
}