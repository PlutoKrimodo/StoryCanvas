#include "RateLimiter.h"

std::mutex RateLimiter::mutex_;
std::unordered_map<std::string, RateLimiter::Window> RateLimiter::windows_;
std::size_t RateLimiter::opsSinceSweep_ = 0;

bool RateLimiter::allow(const std::string &key, int limit, int windowSeconds) {
    return allowAt(key, limit, windowSeconds, std::time(nullptr));
}

bool RateLimiter::allowAt(const std::string &key,
                          int limit,
                          int windowSeconds,
                          std::time_t now) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 周期性清扫：不变量为「映射只保留活跃窗口 + 最多 kSweepInterval 个待清扫项」
    if (++opsSinceSweep_ >= kSweepInterval) {
        opsSinceSweep_ = 0;
        sweepExpiredLocked(now);
    }

    Window &window = windows_[key];
    if (window.windowStart == 0 || now - window.windowStart >= windowSeconds) {
        // 新窗口：重置计数并放行
        window.count = 1;
        window.windowStart = now;
        window.windowSeconds = windowSeconds;
        return true;
    }

    if (limit <= 0) {
        return true;  // 该档位不限制（启动日志会提示）
    }

    ++window.count;
    return window.count <= limit;
}

void RateLimiter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    windows_.clear();
    opsSinceSweep_ = 0;
}

void RateLimiter::sweepExpiredLocked(std::time_t now) {
    for (auto it = windows_.begin(); it != windows_.end();) {
        if (now - it->second.windowStart >= it->second.windowSeconds) {
            it = windows_.erase(it);
        } else {
            ++it;
        }
    }
}
