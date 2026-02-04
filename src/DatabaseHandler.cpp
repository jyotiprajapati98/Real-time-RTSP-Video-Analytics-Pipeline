#include "DatabaseHandler.hpp"
#include <iostream>

DatabaseHandler::DatabaseHandler() : conn(nullptr) {}

DatabaseHandler::~DatabaseHandler() {
    if (conn) {
        PQfinish(conn);
    }
}

bool DatabaseHandler::init(const std::string& connInfo) {
    conn = PQconnectdb(connInfo.c_str());
    std::cout << "[DEBUG] DB init: this=" << this << ", conn=" << conn << std::endl;

    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        conn = nullptr;
        return false;
    }

    // Create Table
    const char* sql = "CREATE TABLE IF NOT EXISTS detections (" \
                      "id SERIAL PRIMARY KEY," \
                      "device_name TEXT," \
                      "class_name TEXT," \
                      "confidence REAL," \
                      "timestamp TEXT," \
                      "frame_path TEXT);";

    PGresult* res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "SQL error: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);

    std::cout << "Connected to PostgreSQL." << std::endl;
    return true;
}

bool DatabaseHandler::logDetection(const std::string& deviceName, const std::string& className, float confidence, const std::string& timestamp, const std::string& framePath) {
    std::cout << "[DEBUG] logDetection: this=" << this << ", conn=" << conn << std::endl;
    if (!conn) {
        std::cerr << "[ERROR] conn is NULL!" << std::endl;
        return false;
    }

    const char* paramValues[5];
    paramValues[0] = deviceName.c_str();
    paramValues[1] = className.c_str();
    std::string confStr = std::to_string(confidence);
    paramValues[2] = confStr.c_str();
    paramValues[3] = timestamp.c_str();
    paramValues[4] = framePath.c_str();

    PGresult* res = PQexecParams(conn,
                                 "INSERT INTO detections (device_name, class_name, confidence, timestamp, frame_path) VALUES ($1, $2, $3, $4, $5)",
                                 5,       // nParams
                                 nullptr, // paramTypes (let server infer)
                                 paramValues,
                                 nullptr, // paramLengths (text)
                                 nullptr, // paramFormats (text)
                                 0);      // resultFormat (text)

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Insert failed: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    std::cout << "[DEBUG] DB Insert OK: " << className << std::endl;
    PQclear(res);
    return true;
}
