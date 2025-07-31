/**
 * MIT License
 *
 * Copyright (c) 2025 Mag1c.H. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * */
#ifndef FLUX_H
#define FLUX_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <list>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace flux {

class Flux {
public:
    enum class Level { DEBUG, INFO, WARN, ERROR };
    struct SourceLocation {
        const char* file = "unknown";
        const char* func = "unknown";
        const size_t line = 0;
    };

private:
    using Buffer = std::list<std::string>;

public:
    ~Flux();
    static Flux& Instance()
    {
        static Flux instance;
        return instance;
    }
    template <typename... Args>
    void log(Level lv, SourceLocation loc, fmt::format_string<Args...> fmt, Args&&... args)
    {
        auto payload = fmt::format("[{}] [FLUX] [{}] {} [{}] [{}]\n", this->FormatAs(std::chrono::system_clock::now()),
                                   this->FormatAs(lv), fmt::format(fmt, std::forward<Args>(args)...),
                                   this->FormatAs(getpid(), std::this_thread::get_id()), this->FormatAs(loc));
        this->Push(std::move(payload));
    }

private:
    Flux() : _worker(&Flux::WorkerLoop, this) {}
    Flux(const Flux&) = delete;
    Flux& operator=(const Flux&) = delete;
    std::string FormatAs(std::chrono::system_clock::time_point tp)
    {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::tm localTm;
        localtime_r(&time, &localTm);
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(tp.time_since_epoch()).count() % 1000000;
        return fmt::format("{:%F %T}.{:06d}", localTm, us);
    }
    std::string FormatAs(pid_t pid, std::thread::id tid)
    {
        return fmt::format("{},{:020d}", pid, std::hash<std::thread::id>{}(tid));
    }
    std::string FormatAs(SourceLocation loc) { return fmt::format("{},{}:{}", loc.func, basename(loc.file), loc.line); }
    std::string_view FormatAs(Level lv)
    {
        switch (lv) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }
    void WorkerLoop();
    void Push(std::string&& msg);

private:
    std::atomic<bool> _stop{false};
    std::chrono::steady_clock::time_point _lastFlush{std::chrono::steady_clock::now()};
    Buffer _frontBuf;
    Buffer _backBuf;
    std::mutex _mtx;
    std::condition_variable _cv;
    std::thread _worker;
};

} // namespace flux

#define FLUX_LOG(lv, fmt, ...)                                                                                         \
    flux::Flux::Instance().log(lv, {__FILE__, __FUNCTION__, __LINE__}, FMT_STRING(fmt), ##__VA_ARGS__)
#define FLUX_DEBUG(fmt, ...) FLUX_LOG(flux::Flux::Level::DEBUG, fmt, ##__VA_ARGS__)
#define FLUX_INFO(fmt, ...) FLUX_LOG(flux::Flux::Level::INFO, fmt, ##__VA_ARGS__)
#define FLUX_WARN(fmt, ...) FLUX_LOG(flux::Flux::Level::WARN, fmt, ##__VA_ARGS__)
#define FLUX_ERROR(fmt, ...) FLUX_LOG(flux::Flux::Level::ERROR, fmt, ##__VA_ARGS__)

#endif
