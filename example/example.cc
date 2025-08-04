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
#include <vector>
#include "flux/flux.h"

int main(int argc, char const* argv[])
{
    std::vector<std::thread> pool;
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        pool.emplace_back([i] {
            for (int j = 0; j < 1000; ++j) {
                FLUX_INFO("Thread({}): {}L.", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    for (auto& t : pool) t.join();
    return 0;
}
