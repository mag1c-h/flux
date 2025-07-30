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

namespace flux {

static constexpr size_t FlushBatchSize = 1024;
static constexpr auto FlushLatency = std::chrono::microseconds(200);

Flux::~Flux()
{
    this->_stop.store(true, std::memory_order_relaxed);
    {
        std::lock_guard lg(this->_mtx);
        this->_frontBuf.swap(this->_backBuf);
        this->_cv.notify_one();
    }
    if (this->_worker.joinable()) { this->_worker.join(); }
}

std::string_view Flux::FormatAs(Level lv)
{
    switch (lv) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO: return "INFO";
    case Level::WARN: return "WARN";
    case Level::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void Flux::WorkerLoop()
{
    Buffer localBuf;
    while (true) {
        {
            std::unique_lock ul(this->_mtx);
            this->_cv.wait(ul, [this] {
                return this->_stop.load(std::memory_order_relaxed) || !this->_backBuf->empty() ||
                       std::chrono::steady_clock::now() - this->_lastFlush >= FlushLatency;
            });
            if (this->_stop.load(std::memory_order_relaxed)) { break; }
            localBuf.swap(*this->_backBuf);
        }
        for (const auto& s : localBuf) { std::fwrite(s.data(), 1, s.size(), stdout); }
        std::fflush(stdout);
        localBuf.clear();
    }
    while (!this->_backBuf->empty()) {
        localBuf.swap(*this->_backBuf);
        for (const auto& s : localBuf) { std::fwrite(s.data(), 1, s.size(), stdout); }
        std::fflush(stdout);
        localBuf.clear();
    }
}

void Flux::Push(std::string&& msg)
{
    auto now = std::chrono::steady_clock::now();
    std::lock_guard lg(this->_mtx);
    this->_frontBuf->push_back(std::move(msg));
    bool byCount = this->_frontBuf->size() >= FlushBatchSize;
    bool byTime = now - this->_lastFlush >= FlushLatency;
    if (byCount || byTime) {
        this->_frontBuf.swap(this->_backBuf);
        this->_lastFlush = now;
        this->_cv.notify_one();
        this->_frontBuf->clear();
    }
}

} // namespace flux
