#include "pawnshop/measurement_db.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdio>

using namespace std;
using system_clock = std::chrono::system_clock;
using seconds = std::chrono::seconds;

// TODO: Lots and lots of error handling
namespace pawnshop {

MeasurementDb::MeasurementDb(const string& db_path) {
    sqlite3_open_v2(db_path.c_str(), &db,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    sqlite3_exec(db,
                 u8"CREATE TABLE IF NOT EXISTS measurements ("
                 u8"    dirtyWeight REAL NOT NULL,"
                 u8"    cleanWeight REAL NOT NULL,"
                 u8"    submergedWeight REAL NOT NULL,"
                 u8"    density REAL NOT NULL,"
                 u8"    startTime INTEGER NOT NULL,"
                 u8"    endTime INTEGER NOT NULL,"
                 u8"    productId INTEGER NOT NULL"
                 u8");",
                 nullptr, nullptr, nullptr);
}

MeasurementDb::~MeasurementDb() { sqlite3_close_v2(db); }

/**
 * Bind all values except rowid to a statement
 *
 * @param offset: Offset column index in binds
 * @returns last column index
 */
inline int64_t bindMeasurementValues(sqlite3_stmt* stmt, const Measurement& m,
                                     int64_t offset = 0) {
    int64_t column_idx = 1 + offset;
    sqlite3_bind_double(stmt, column_idx++, m.dirty_weight);
    sqlite3_bind_double(stmt, column_idx++, m.clean_weight);
    sqlite3_bind_double(stmt, column_idx++, m.submerged_weight);
    sqlite3_bind_double(stmt, column_idx++, m.density);
    int64_t epoch = std::chrono::time_point_cast<seconds>(m.start_time)
                        .time_since_epoch()
                        .count();
    sqlite3_bind_int64(stmt, column_idx++, epoch);
    epoch = std::chrono::time_point_cast<seconds>(m.end_time)
                .time_since_epoch()
                .count();
    sqlite3_bind_int64(stmt, column_idx++, epoch);
    sqlite3_bind_int64(stmt, column_idx++, m.product_id);
    return column_idx;
}

int64_t MeasurementDb::insert(const Measurement& m) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
                       u8"INSERT INTO measurements VALUES ($dirt, $clean, "
                       u8"$sub, $den, $start, $end, $product) RETURNING rowid",
                       -1, &stmt, nullptr);

    bindMeasurementValues(stmt, m);
    sqlite3_step(stmt);
    int64_t id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return id;
}

void MeasurementDb::update(const Measurement& m) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(
        db,
        u8"UPDATE measurements SET dirtyWeight=$dirt, cleanWeight=$clean, "
        u8"submergedWeight=$sub, density=$den, startTime=$start, endTime=$end, "
        u8"productId=$product WHERE rowid = $id",
        -1, &stmt, nullptr);

    int64_t column_idx = bindMeasurementValues(stmt, m);
    sqlite3_bind_int64(stmt, column_idx++, m.id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

inline Measurement getMeasurementRow(sqlite3_stmt* stmt) {
    Measurement m;
    m.id = sqlite3_column_int64(stmt, 0);
    m.dirty_weight = sqlite3_column_double(stmt, 1);
    m.clean_weight = sqlite3_column_double(stmt, 2);
    m.submerged_weight = sqlite3_column_double(stmt, 3);
    m.density = sqlite3_column_double(stmt, 4);
    int64_t epoch = sqlite3_column_int64(stmt, 5);
    m.start_time = system_clock::time_point{seconds{epoch}};
    epoch = sqlite3_column_int64(stmt, 6);
    m.end_time = system_clock::time_point{seconds{epoch}};
    m.product_id = sqlite3_column_int64(stmt, 7);
    return m;
}

Measurement MeasurementDb::findById(int64_t id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
                       u8"SELECT rowid,* FROM measurements WHERE rowid = $id",
                       -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    Measurement res = getMeasurementRow(stmt);

    sqlite3_finalize(stmt);
    return res;
}

vector<Measurement> MeasurementDb::findByProductId(int64_t productId) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(
        db, u8"SELECT rowid,* FROM measurements WHERE productId = $product", -1,
        &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, productId);
    vector<Measurement> measurements;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Measurement m = getMeasurementRow(stmt);
        measurements.push_back(move(m));
    }
    return measurements;
}

vector<Measurement> MeasurementDb::getAll() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, u8"SELECT rowid,* FROM measurements", -1, &stmt,
                       nullptr);
    vector<Measurement> measurements;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Measurement m = getMeasurementRow(stmt);
        measurements.push_back(move(m));
    }
    return measurements;
}

TEST_CASE("MeasurementsDB") {
    string db_path = "./testing.sqlite3";
    auto db = new MeasurementDb(db_path);

    Measurement m;
    m.start_time = std::chrono::time_point_cast<seconds>(system_clock::now());
    m.density = 1;
    m.product_id = 1;

    SUBCASE("Insertion") {
        m.id = db->insert(m);
        auto m2 = db->findById(m.id);

        CHECK(m.start_time == m2.start_time);
        CHECK(m.density == m2.density);
    }

    SUBCASE("Update") {
        m.id = db->insert(m);

        m.density = 2;
        db->update(m);
        auto m2 = db->findById(m.id);

        CHECK(m.density == m2.density);
    }

    SUBCASE("Find by Product id") {
        db->insert(m);
        db->insert(m);
        m.product_id = 2;
        db->insert(m);

        auto ms = db->findByProductId(1);
        CHECK(ms.size() == 2);
    }

    delete db;
    remove(db_path.c_str());
};

}  // namespace pawnshop
