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

// 期货交易时间配置
void setupFuturesScheduler(AutoScheduler& scheduler) {
    // === 日盘时段 ===
    // 周一至周五 08:50 启动
    scheduler.addRule({ 8, 50, AutoScheduler::Action::START, {1,2,3,4,5}, "Day start" });
    // 周一至周五 15:59 停止
    scheduler.addRule({ 15, 59, AutoScheduler::Action::STOP, {1,2,3,4,5}, "Day stop" });

    // === 夜盘时段 ===
    // 周一至周五 20:50 启动（前一天夜盘）
    scheduler.addRule({ 20, 50, AutoScheduler::Action::START, {1,2,3,4,5}, "Night start" });
    // 周二至周六 02:35 停止（跨夜）
    scheduler.addRule({ 2, 35, AutoScheduler::Action::STOP, {2,3,4,5,6}, "Night stop" });

    // === 周末兜底 ===
    // 周日 00:00 强制停止
    scheduler.addRule({ 0, 0, AutoScheduler::Action::STOP, {7}, "Sunday fallback" });
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
    setupFuturesScheduler(scheduler);


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