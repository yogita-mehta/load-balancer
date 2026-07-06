#pragma once
#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

// Simple thread-safe file logger. One instance per log file; the LoadBalancer
// application wires up application.log, errors.log, and performance.log.
class Logger {
public:
    explicit Logger(const std::string& path) : file_(path, std::ios::app) {}

    void log(const std::string& level, const std::string& message) {
        std::lock_guard<std::mutex> lk(mutex_);
        file_ << "[" << timestamp() << "] [" << level << "] " << message << std::endl;
    }

    void info(const std::string& msg) { log("INFO", msg); }
    void warn(const std::string& msg) { log("WARN", msg); }
    void error(const std::string& msg) { log("ERROR", msg); }

private:
    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    std::mutex mutex_;
    std::ofstream file_;
};
