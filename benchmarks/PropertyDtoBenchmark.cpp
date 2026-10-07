#include <benchmark/benchmark.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/features/property/controller/PropertyDto.h"
#include "lintel/features/property/controller/PropertiesDto.h"
#include "lintel/features/property/models/Property.h"

static std::shared_ptr<PropertyBase> MakeTestProperty(int i) {
    return std::make_shared<Property<int>>(
        "prop_" + std::to_string(i),
        "instance_" + std::to_string(i % 10),
        "class_" + std::to_string(i % 5),
        "process_" + std::to_string(i % 3),
        i * 100,
        "description for property " + std::to_string(i),
        i % 2 == 0);
}

static void BM_PropertyDto_Construct(benchmark::State &state) {
    auto prop = MakeTestProperty(0);
    for (auto _ : state) {
        PropertyDto dto(prop);
    }
}
BENCHMARK(BM_PropertyDto_Construct);

static void BM_PropertyDto_Serialize(benchmark::State &state) {
    auto prop = MakeTestProperty(42);
    PropertyDto dto(prop);
    for (auto _ : state) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        dto.serialize(&writer);
    }
}
BENCHMARK(BM_PropertyDto_Serialize);

static void BM_PropertyDto_Deserialize(benchmark::State &state) {
    auto prop = MakeTestProperty(42);
    PropertyDto dto(prop);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    dto.serialize(&writer);
    std::string json = buffer.GetString();

    for (auto _ : state) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());
        PropertyDto result;
        result.deserialize(doc);
    }
}
BENCHMARK(BM_PropertyDto_Deserialize);

static void BM_PropertiesDto_Serialize10(benchmark::State &state) {
    std::vector<std::shared_ptr<PropertyBase>> props;
    for (int i = 0; i < 10; ++i) {
        props.push_back(MakeTestProperty(i));
    }
    PropertiesDto dto(props);
    for (auto _ : state) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        dto.serialize(&writer);
    }
}
BENCHMARK(BM_PropertiesDto_Serialize10);

static void BM_PropertiesDto_Serialize100(benchmark::State &state) {
    std::vector<std::shared_ptr<PropertyBase>> props;
    for (int i = 0; i < 100; ++i) {
        props.push_back(MakeTestProperty(i));
    }
    PropertiesDto dto(props);
    for (auto _ : state) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        dto.serialize(&writer);
    }
}
BENCHMARK(BM_PropertiesDto_Serialize100);

static void BM_PropertiesDto_Deserialize10(benchmark::State &state) {
    std::vector<std::shared_ptr<PropertyBase>> props;
    for (int i = 0; i < 10; ++i) {
        props.push_back(MakeTestProperty(i));
    }
    PropertiesDto dto(props);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    dto.serialize(&writer);
    std::string json = buffer.GetString();

    for (auto _ : state) {
        PropertiesDto result;
        result.deserialize(json);
    }
}
BENCHMARK(BM_PropertiesDto_Deserialize10);

static void BM_PropertyDto_ConstructBatch(benchmark::State &state) {
    const int N = state.range(0);
    std::vector<std::shared_ptr<PropertyBase>> props;
    for (int i = 0; i < N; ++i) {
        props.push_back(MakeTestProperty(i));
    }

    for (auto _ : state) {
        std::vector<PropertyDto> dtos;
        dtos.reserve(N);
        for (auto &p : props) {
            dtos.emplace_back(p);
        }
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_PropertyDto_ConstructBatch)->Range(8, 1024);
