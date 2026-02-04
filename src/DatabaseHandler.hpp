#pragma once

#include <string>
#include <libpq-fe.h>

class DatabaseHandler {
public:
    DatabaseHandler();
    ~DatabaseHandler();

    // Connection string e.g. "postgresql://user:password@localhost/mydb"
    bool init(const std::string& connInfo);
    bool logDetection(const std::string& deviceName, const std::string& className, float confidence, const std::string& timestamp, const std::string& framePath);

private:
    PGconn* conn = nullptr;
};
