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
#include "flux/flux.h"
#include <fmt/chrono.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace flux {

static constexpr size_t FlushBatchSize = 1024;
static constexpr auto FlushLatency = std::chrono::milliseconds(10);

Flux::~Flux()
{
    this->_stop.store(true, std::memory_order_relaxed);
    {
        std::lock_guard lg(this->_mtx);
        this->_backBuf.splice(this->_backBuf.end(), this->_frontBuf);
        this->_cv.notify_one();
    }
    if (this->_worker.joinable()) { this->_worker.join(); }
}

void Flux::WorkerLoop()
{
    Buffer localBuf;
    while (true) {
        {
            std::unique_lock ul(this->_mtx);
            auto triggered = this->_cv.wait_for(ul, FlushLatency, [this] {
                return this->_stop.load(std::memory_order_relaxed) || !this->_backBuf.empty();
            });
            if (this->_stop.load(std::memory_order_relaxed)) { break; }
            localBuf.splice(localBuf.end(), this->_backBuf);
            if (!triggered) { localBuf.splice(localBuf.end(), this->_frontBuf); }
        }
        if (localBuf.empty()) { continue; }
        for (const auto& s : localBuf) { std::fwrite(s.data(), 1, s.size(), stdout); }
        std::fflush(stdout);
        localBuf.clear();
    }
    while (!this->_backBuf.empty()) {
        localBuf.splice(localBuf.end(), this->_backBuf);
        for (const auto& s : localBuf) { std::fwrite(s.data(), 1, s.size(), stdout); }
        std::fflush(stdout);
        localBuf.clear();
    }
}

void Flux::Push(Level&& lv, SourceLocation&& loc, std::string&& msg)
{
    using namespace std::chrono;
    static const char* lvStrs[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    static const size_t pid = static_cast<size_t>(getpid());
    static thread_local const size_t tid = syscall(SYS_gettid);
    static thread_local seconds lastSec{0};
    static thread_local char datetime[32];
    auto systemNow = system_clock::now();
    auto currentSec = time_point_cast<seconds>(systemNow);
    if (lastSec != currentSec.time_since_epoch()) {
        auto systemTime = system_clock::to_time_t(systemNow);
        std::tm systemTm;
        localtime_r(&systemTime, &systemTm);
        fmt::format_to_n(datetime, sizeof(datetime), "{:%F %T}", systemTm);
        lastSec = currentSec.time_since_epoch();
    }
    auto us = duration_cast<microseconds>(systemNow - currentSec).count();
    auto payload = fmt::format("[{}.{:06d}] [FLUX] [{}] {} [{},{}] [{},{}:{}]\n", datetime, us,
                               lvStrs[fmt::underlying(lv)], msg, pid, tid, loc.func, basename(loc.file), loc.line);
    auto steadyNow = steady_clock::now();
    std::lock_guard lg(this->_mtx);
    this->_frontBuf.push_back(std::move(payload));
    bool byCount = this->_frontBuf.size() >= FlushBatchSize;
    bool byTime = steadyNow - this->_lastFlush >= FlushLatency;
    if (byCount || byTime) {
        this->_backBuf.splice(this->_backBuf.end(), this->_frontBuf);
        this->_lastFlush = steadyNow;
        this->_cv.notify_one();
    }
}

} // namespace flux
