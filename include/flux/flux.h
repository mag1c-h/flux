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
#include <fmt/core.h>
#include <list>
#include <mutex>
#include <thread>

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
    void log(Level&& lv, SourceLocation&& loc, fmt::format_string<Args...> fmt, Args&&... args)
    {
        this->Push(std::move(lv), std::move(loc), fmt::format(fmt, std::forward<Args>(args)...));
    }

private:
    Flux() : _worker(&Flux::WorkerLoop, this) {}
    Flux(const Flux&) = delete;
    Flux& operator=(const Flux&) = delete;
    void WorkerLoop();
    void Push(Level&& lv, SourceLocation&& loc, std::string&& msg);

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
