#pragma once

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace pawnshop {

struct Measurement {
    // Automatically generated id for measurement
    int64_t id;
    // Id of product that is being measured
    int64_t productId;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime;
    double dirtyWeight;
    double cleanWeight;
    double submergedWeight;
    double density;
};

class MeasurementDb {
public:
    MeasurementDb(const std::string& dbPath);
    MeasurementDb(const MeasurementDb&) = delete;
    ~MeasurementDb();
    /**
     * @returns Id of new measurement
     */
    int64_t insert(const Measurement& m);
    void update(const Measurement& m);
    Measurement findById(int64_t id);
    std::vector<Measurement> findByProductId(int64_t productId);
    std::vector<Measurement> getAll();

private:
    sqlite3* db = nullptr;
};

}  // namespace pawnshop
