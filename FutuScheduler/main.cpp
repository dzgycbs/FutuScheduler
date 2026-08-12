// main.cpp
#include "Logger.h"
#include "AutoScheduler.h"
#include "MarketManager.h"
#include <signal.h>
#include <atomic>

std::atomic<bool> g_shutdown(false);

void signalHandler(int sig) {
    LOG_INFO("Received signal {}, shutting down...", sig);
    g_shutdown = true;
}

int main() {
    // 1. 初始化日志
    Logger::getInstance().init("logs", "market_gateway");
    LOG_INFO("=== Market Gateway Starting ===");

    // 2. 设置信号处理（优雅退出）
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 3. 创建业务管理器
    MarketManager manager;

    // 4. 创建调度器，配置规则
    AutoScheduler scheduler;

    // 规则1: 工作日 08:50 启动（提前10分钟准备）
    scheduler.addRule({
        8, 50,                           // 08:50
        AutoScheduler::Action::START,    // 启动
        {1, 2, 3, 4, 5},                 // 周一到周五
        "Weekday start at 08:50"
        });

    // 规则2: 工作日 15:30 停止
    scheduler.addRule({
        15, 30,                          // 15:30
        AutoScheduler::Action::STOP,     // 停止
        {1, 2, 3, 4, 5},                 // 周一到周五
        "Weekday stop at 15:30"
        });

    // 规则3: 周末停止（跨周末兜底）
    scheduler.addRule({
        0, 0,                            // 00:00
        AutoScheduler::Action::STOP,     // 停止
        {6, 7},                          // 周六周日
        "Weekend stop at 00:00"
        });

    // 设置回调
    scheduler.setOnStartBusiness([&]() {
        manager.start();
        });
    scheduler.setOnStopBusiness([&]() {
        manager.stop();
        });

    // 5. 启动调度器
    scheduler.start();

    // 6. 主循环（等待退出信号）
    LOG_INFO("Gateway running. Press Ctrl+C to exit.");

    while (!g_shutdown) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 7. 清理
    LOG_INFO("Shutting down...");
    scheduler.stop();
    manager.stop();
    Logger::getInstance().shutdown();

    LOG_INFO("=== Market Gateway Stopped ===");
    return 0;
}