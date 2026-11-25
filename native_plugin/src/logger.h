#pragma once

#include <mutex>
#include <string>

namespace telemetry_plugin {

class Logger {
public:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool Open();
    void Close();
    void Log(const char* text);

    static std::string LogDirectory();
    static std::string LogPath();

private:
    void* stream_ = nullptr;
    std::mutex mutex_;
};

} // namespace telemetry_plugin
