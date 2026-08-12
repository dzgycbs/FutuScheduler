// MarketManager.h
#pragma once

#include <atomic>
#include <thread>

class MarketManager {
public:
    MarketManager();
    ~MarketManager();

    // 禁止拷贝
    MarketManager(const MarketManager&) = delete;
    MarketManager& operator=(const MarketManager&) = delete;

    // 启动业务（行情订阅 + 转发）
    void start();

    // 停止业务（取消订阅 + 停止转发）
    void stop();

    // 检查业务是否运行中
    bool isRunning() const { return m_running; }

private:
    // 实际的工作线程
    void workThread();

private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_shouldStop;
    std::thread m_workThread;
};