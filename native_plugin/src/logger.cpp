#include "logger.h"

#include <Windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>

namespace telemetry_plugin {

static std::string EnvironmentValue(const char* name) {
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr) {
        return {};
    }
    std::string value(buffer);
    free(buffer);
    return value;
}

std::string Logger::LogDirectory() {
    std::string temp = EnvironmentValue("TEMP");
    if (temp.empty()) {
        temp = EnvironmentValue("TMP");
    }
    if (temp.empty()) {
        temp = ".";
    }

    std::string directory = temp;
    if (!directory.empty() && directory.back() != '\\' && directory.back() != '/') {
        directory += '\\';
    }
    return directory + "ETS2TelemetryPlugin";
}

std::string Logger::LogPath() {
    std::string directory = LogDirectory();
    if (!directory.empty() && directory.back() != '\\' && directory.back() != '/') {
        directory += '\\';
    }
    return directory + "telemetry_plugin.log";
}

Logger::Logger() = default;

Logger::~Logger() {
    Close();
}

bool Logger::Open() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (stream_ != nullptr) {
        return true;
    }

    const std::string directory = LogDirectory();
    if (!CreateDirectoryA(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }

    try {
        auto* stream = new std::ofstream(LogPath(), std::ios::out | std::ios::app);
        if (!stream->is_open()) {
            delete stream;
            return false;
        }
        stream_ = stream;
        return true;
    }
    catch (...) {
        return false;
    }
}

void Logger::Close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (stream_ == nullptr) {
        return;
    }

    try {
        auto* stream = static_cast<std::ofstream*>(stream_);
        stream->flush();
        stream->close();
        delete stream;
    }
    catch (...) {
    }
    stream_ = nullptr;
}

void Logger::Log(const char* text) {
    if (text == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_ == nullptr) {
        return;
    }

    try {
        SYSTEMTIME now;
        GetLocalTime(&now);

        char timestamp[32];
        std::snprintf(
            timestamp,
            sizeof(timestamp),
            "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            now.wYear,
            now.wMonth,
            now.wDay,
            now.wHour,
            now.wMinute,
            now.wSecond,
            now.wMilliseconds);

        auto* stream = static_cast<std::ofstream*>(stream_);
        *stream << '[' << timestamp << "] " << text << '\n';
        stream->flush();
    }
    catch (...) {
    }
}

} // namespace telemetry_plugin
