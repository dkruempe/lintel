#include <iostream>
#include <chrono>
#include <date/date.h>
#include <magic_enum.hpp>


class Features {
    std::vector<std::string> features{};

public:

    enum Value {
        Base,
        Http,
        Property,
        Cli
    };

    template<typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
    void registerFeature(T x) {
        features.push_back(std::string(magic_enum::enum_name(x)));
    };

    Features() {
        auto names = magic_enum::enum_names<Value>();
        for (auto &iter: names) {
            features.emplace_back(iter);
        }
    }

    std::vector<std::string> allOf() {
        return features;
    }
};

int main(int argc, char *argv[]) {
    Features features{};
    enum NEW_FEATURES {
        Test
    };
    features.registerFeature(Test);
    auto vec = features.allOf();
    for (const auto &iter : vec) {
        std::cout << iter << "\n";
    }
    return 0;
}