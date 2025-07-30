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
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>

namespace flux {

class Flux {
public:
    enum class Level { DEBUG, INFO, WARN, ERROR };
    struct SourceLocation {
        const char* file = "unknown";
        const char* func = "unknown";
        const size_t line = 0;
    };

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
        auto payload =
            fmt::format("[FLUX] [{:%Y-%m-%d %H:%M:%S}] [{}] [{}] {} [{},{}:{}]\n", std::chrono::system_clock::now(),
                        this->FormatAs(lv), this->ThreadId(), fmt::format(fmt, std::forward<Args>(args)...), loc.func,
                        basename(loc.file), loc.line);
        this->Push(std::move(payload));
    }

private:
    Flux() : _worker(&Flux::WorkerLoop, this) {}
    Flux(const Flux&) = delete;
    Flux& operator=(const Flux&) = delete;
    std::string ThreadId()
    {
        return fmt::format("{:08},{:020}", getpid(), std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }
    std::string_view FormatAs(Level lv);
    void WorkerLoop();
    void Push(std::string&& msg);

private:
    using Buffer = std::vector<std::string>;
    std::unique_ptr<Buffer> _frontBuf = std::make_unique<Buffer>();
    std::unique_ptr<Buffer> _backBuf = std::make_unique<Buffer>();
    std::atomic<bool> _stop{false};
    std::chrono::steady_clock::time_point _lastFlush{std::chrono::steady_clock::now()};
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
