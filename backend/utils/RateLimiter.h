#pragma once

#include <cstddef>
#include <ctime>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * 内存固定窗口限流计数器（MVP 单机版，见 docs/10 §4.1、docs/16 §7.4）。
 *
 * 设计取舍：
 *   - 固定窗口（而非滑动窗口）：实现简单、无额外依赖；窗口边界处允许
 *     瞬时两倍流量，对演示环境足够（接入 Redis 后可平滑升级）；
 *   - 键空间由调用方定义（如 "ip:127.0.0.1"、"user:<uuid>"），本类不关
 *     心业务语义；
 *   - 计数保存在进程内存，重启清零；过期窗口会被周期性清扫，避免
 *     唯一 IP 造访造成映射无限增长。
 */
class RateLimiter {
public:
    /**
     * 记录一次访问并判断是否允许。
     *
     * @param key           限流键（调用方需自带维度前缀）
     * @param limit         窗口内允许的最大次数；<=0 表示该键不限制
     * @param windowSeconds 窗口长度（秒）
     * @return true 允许；false 超限
     */
    static bool allow(const std::string &key, int limit, int windowSeconds);

    /**
     * 与 allow 相同，但允许注入时间基准。
     * 仅供单元测试使用（避免依赖真实时钟），生产代码请调用 allow。
     */
    static bool allowAt(const std::string &key,
                        int limit,
                        int windowSeconds,
                        std::time_t now);

    /** 清空全部计数（仅供测试使用） */
    static void reset();

private:
    struct Window {
        int count = 0;
        std::time_t windowStart = 0;
        int windowSeconds = 60;
    };

    /** 清除已过期的窗口；调用方必须已持有 mutex_ */
    static void sweepExpiredLocked(std::time_t now);

    static std::mutex mutex_;
    static std::unordered_map<std::string, Window> windows_;
    /** 距上次清扫的调用次数（限流为高频路径，不能每次全表扫描） */
    static std::size_t opsSinceSweep_;

    /** 每 N 次访问触发一次过期清扫 */
    static constexpr std::size_t kSweepInterval = 1024;
};
