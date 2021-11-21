#pragma once

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
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

struct CalibrationInfo {
    double caret_weight;
    double caret_submerged_weight;
};

class Db {
public:
    Db(const std::string& db_path);
    Db(const Db&) = delete;
    ~Db();

    void updateCalibrationInfo(const CalibrationInfo& i);
    std::optional<CalibrationInfo> getCalibrationInfo();

    /**
     * @returns Id of new measurement
     */
    int64_t insertMeasurement(const Measurement& m);
    void updateMeasurement(const Measurement& m);
    std::optional<Measurement> findMeasurementById(int64_t id);
    std::vector<Measurement> findMeasurementsByProductId(int64_t product_id);
    std::vector<Measurement> getAllMeasurements();

private:
    sqlite3* db = nullptr;
};

}  // namespace pawnshop
