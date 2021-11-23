#include "pawnshop/db.hpp"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdio>

using namespace std;
using system_clock = std::chrono::system_clock;
using seconds = std::chrono::seconds;
using json = nlohmann::json;

// TODO: Lots and lots of error handling
namespace pawnshop {

void to_json(json& j, const Measurement& m) {
    j = {{"id", m.id},
         {"product_id", m.product_id},
         {"start_time", std::chrono::time_point_cast<seconds>(m.start_time)
                            .time_since_epoch()
                            .count()},
         {"end_time", std::chrono::time_point_cast<seconds>(m.end_time)
                          .time_since_epoch()
                          .count()},
         {"dirty_weight", m.dirty_weight},
         {"clean_weight", m.clean_weight},
         {"submerged_weight", m.submerged_weight},
         {"density", m.density}};
}

void from_json(const nlohmann::json& j, Measurement& m) {
    j.at("id").get_to(m.id);
    j.at("product_id").get_to(m.product_id);
    auto epoch = j.at("start_time").get<int64_t>();
    m.start_time = system_clock::time_point{seconds{epoch}};
    epoch = j.at("end_time").get<int64_t>();
    m.end_time = system_clock::time_point{seconds{epoch}};
    j.at("dirty_weight").get_to(m.dirty_weight);
    j.at("clean_weight").get_to(m.clean_weight);
    j.at("submerged_weight").get_to(m.submerged_weight);
    j.at("density").get_to(m.density);
}

DbConfig::DbConfig(const toml::table& table) {
    path = table["path"].value<string>().value();
}

Db::Db(const string& db_path) {
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
                 u8");"
                 u8"CREATE TABLE IF NOT EXISTS calibrationInfo ("
                 u8"    caretWeight REAL NOT NULL,"
                 u8"    caretSubmergedWeight REAL NOT NULL"
                 u8");",
                 nullptr, nullptr, nullptr);
}

Db::~Db() { sqlite3_close_v2(db); }

void Db::updateCalibrationInfo(const CalibrationInfo& i) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(
        db,
        u8"INSERT OR REPLACE INTO calibrationInfo (rowid, "
        u8"caretWeight, caretSubmergedWeight) VALUES (1, $weight, $submerged);",
        -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, i.caret_weight);
    sqlite3_bind_double(stmt, 2, i.caret_submerged_weight);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

inline CalibrationInfo getCalibrationInfoRow(sqlite3_stmt* stmt) {
    CalibrationInfo i;
    i.caret_weight = sqlite3_column_double(stmt, 0);
    i.caret_submerged_weight = sqlite3_column_double(stmt, 1);
    return i;
}

optional<CalibrationInfo> Db::getCalibrationInfo() {
    optional<CalibrationInfo> res;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, u8"SELECT * FROM calibrationInfo WHERE rowid = 1;",
                       -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        res = getCalibrationInfoRow(stmt);
    } else {
        res = {};
    }
    sqlite3_finalize(stmt);
    return res;
}

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

int64_t Db::insertMeasurement(const Measurement& m) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
                       u8"INSERT INTO measurements VALUES ($dirt, $clean, "
                       u8"$sub, $den, $start, $end, $product) RETURNING rowid;",
                       -1, &stmt, nullptr);

    bindMeasurementValues(stmt, m);
    sqlite3_step(stmt);
    int64_t id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return id;
}

void Db::updateMeasurement(const Measurement& m) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(
        db,
        u8"UPDATE measurements SET dirtyWeight=$dirt, cleanWeight=$clean, "
        u8"submergedWeight=$sub, density=$den, startTime=$start, endTime=$end, "
        u8"productId=$product WHERE rowid = $id;",
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

optional<Measurement> Db::findMeasurementById(int64_t id) {
    optional<Measurement> res;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
                       u8"SELECT rowid,* FROM measurements WHERE rowid = $id;",
                       -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        res = getMeasurementRow(stmt);
    } else {
        res = {};
    }
    sqlite3_finalize(stmt);
    return res;
}

vector<Measurement> Db::findMeasurementsByProductId(int64_t productId) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(
        db, u8"SELECT rowid,* FROM measurements WHERE productId = $product;",
        -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, productId);
    vector<Measurement> measurements;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Measurement m = getMeasurementRow(stmt);
        measurements.push_back(move(m));
    }
    return measurements;
}

vector<Measurement> Db::getAllMeasurements() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, u8"SELECT rowid,* FROM measurements;", -1, &stmt,
                       nullptr);
    vector<Measurement> measurements;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Measurement m = getMeasurementRow(stmt);
        measurements.push_back(move(m));
    }
    return measurements;
}

TEST_CASE("DB") {
    string db_path = "./testing.sqlite3";
    auto db = new Db(db_path);

    SUBCASE("CaretWeight") {
        CalibrationInfo i;
        i.caret_weight = 10;
        i.caret_submerged_weight = 0.1;

        SUBCASE("NotDefined") {
            auto i = db->getCalibrationInfo();

            CHECK(!i.has_value());
        }
        SUBCASE("Update") {
            db->updateCalibrationInfo(i);
            auto i2 = db->getCalibrationInfo();

            CHECK(i2->caret_weight == i.caret_weight);
            CHECK(i2->caret_submerged_weight == i.caret_submerged_weight);
        }
    }

    SUBCASE("Measurements") {
        Measurement m;
        m.start_time =
            std::chrono::time_point_cast<seconds>(system_clock::now());
        m.density = 1;
        m.product_id = 1;

        SUBCASE("Insertion") {
            m.id = db->insertMeasurement(m);
            auto m2 = db->findMeasurementById(m.id);

            CHECK(m.start_time == m2->start_time);
            CHECK(m.density == m2->density);
        }

        SUBCASE("Update") {
            m.id = db->insertMeasurement(m);

            m.density = 2;
            db->updateMeasurement(m);
            auto m2 = db->findMeasurementById(m.id);

            CHECK(m.density == m2->density);
        }

        SUBCASE("Find by Product id") {
            db->insertMeasurement(m);
            db->insertMeasurement(m);
            m.product_id = 2;
            db->insertMeasurement(m);

            auto ms = db->findMeasurementsByProductId(1);
            CHECK(ms.size() == 2);
        }
    }

    delete db;
    remove(db_path.c_str());
};

}  // namespace pawnshop
