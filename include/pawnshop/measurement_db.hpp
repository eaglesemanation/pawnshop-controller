#pragma once

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace pawnshop {

struct Measurement {
    // Automatically generated id for measurement
    int64_t id;
    // Id of product that is being measured
    int64_t product_id;
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> end_time;
    double dirty_weight;
    double clean_weight;
    double submerged_weight;
    double density;
};

void to_json(nlohmann::json& j, const Measurement& p);
void from_json(const nlohmann::json& j, Measurement& p);

class MeasurementDb {
public:
    MeasurementDb(const std::string& db_path);
    MeasurementDb(const MeasurementDb&) = delete;
    ~MeasurementDb();
    /**
     * @returns Id of new measurement
     */
    int64_t insert(const Measurement& m);
    void update(const Measurement& m);
    Measurement findById(int64_t id);
    std::vector<Measurement> findByProductId(int64_t product_id);
    std::vector<Measurement> getAll();

private:
    sqlite3* db = nullptr;
};

}  // namespace pawnshop
