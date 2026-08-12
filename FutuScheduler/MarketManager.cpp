// MarketManager.cpp
#include "MarketManager.h"
#include "Logger.h"

MarketManager::MarketManager()
    : m_running(false)
    , m_shouldStop(false) {
}

MarketManager::~MarketManager() {
    stop();
}

void MarketManager::start() {
    if (m_running) {
        LOG_WARN("MarketManager already running");
        return;
    }

    LOG_INFO("=== Starting market subscription and forwarding ===");

    // TODO: 这里写你的实际启动逻辑
    // - 连接行情源（如：CTP、券商API等）
    // - 订阅品种列表
    // - 启动转发到中间件（如：Redis、Kafka、MQ等）

    m_shouldStop = false;
    m_running = true;
    m_workThread = std::thread(&MarketManager::workThread, this);

    LOG_INFO("MarketManager started");
}

void MarketManager::stop() {
    if (!m_running) {
        return;
    }

    LOG_INFO("=== Stopping market subscription and forwarding ===");

    // 通知工作线程停止
    m_shouldStop = true;
    m_running = false;

    if (m_workThread.joinable()) {
        m_workThread.join();
    }

    // TODO: 这里写你的实际停止逻辑
    // - 取消订阅
    // - 断开行情源连接
    // - 清理资源
    // - 发送最后的统计信息

    LOG_INFO("MarketManager stopped");
}

void MarketManager::workThread() {
    LOG_DEBUG("MarketManager work thread started");

    while (!m_shouldStop) {
        // TODO: 这里写你的主业务循环
        // 例如：接收行情 -> 处理 -> 转发

        // 模拟业务处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_DEBUG("MarketManager work thread exited");
}