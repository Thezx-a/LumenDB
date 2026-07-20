#pragma once
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace minikv {
namespace utils {

enum class LogLevel { DEBUG, INFO, WARN, ERROR };

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) file_.close();
        file_.open(path, std::ios::app);
    }

    void log(LogLevel level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S",
                      std::localtime(&time));
        const char* levelStr = level == LogLevel::DEBUG ? "DEBUG" :
                               level == LogLevel::INFO  ? "INFO " :
                               level == LogLevel::WARN  ? "WARN " : "ERROR";
        std::string line = std::string("[") + timeBuf + "] [" + levelStr + "] " + msg + "\n";
        if (file_.is_open()) file_ << line;
        else std::clog << line;
    }

private:
    Logger() = default;
    std::ofstream file_;
    std::mutex mutex_;
};

#define LOG_INFO(msg)  ::minikv::utils::Logger::instance().log(::minikv::utils::LogLevel::INFO, msg)
#define LOG_WARN(msg)  ::minikv::utils::Logger::instance().log(::minikv::utils::LogLevel::WARN, msg)
#define LOG_ERROR(msg) ::minikv::utils::Logger::instance().log(::minikv::utils::LogLevel::ERROR, msg)

}  // namespace utils
}  // namespace minikv
