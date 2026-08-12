// AutoScheduler.h
#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <string>

class AutoScheduler {
public:
    enum class Action {
        START,  // 启动业务（开始转发）
        STOP,   // 停止业务（停止转发）
        NONE    // 无操作
    };

    struct ScheduleRule {
        int hour;                       // 小时 (0-23)
        int minute;                     // 分钟 (0-59)
        Action action;                  // 要执行的动作
        std::vector<int> weekdays;      // 星期几生效 (1=周一, 7=周日)，空表示每天
        std::string desc;               // 描述
    };

    AutoScheduler();
    ~AutoScheduler();

    // 禁止拷贝
    AutoScheduler(const AutoScheduler&) = delete;
    AutoScheduler& operator=(const AutoScheduler&) = delete;

    // 添加调度规则
    void addRule(const ScheduleRule& rule);

    // 设置业务启动回调
    void setOnStartBusiness(std::function<void()> callback);

    // 设置业务停止回调
    void setOnStopBusiness(std::function<void()> callback);

    // 启动调度器（在独立线程中运行）
    void start();

    // 停止调度器
    void stop();

    // 获取当前业务运行状态
    bool isBusinessRunning() const;

private:
    // 调度器主循环
    void run();

    // 检查并执行调度规则
    void checkAndExecute();

    // 判断当前时间是否匹配规则
    bool matchRule(const ScheduleRule& rule, const std::tm& tm) const;

private:
    std::vector<ScheduleRule> m_rules;
    std::atomic<bool> m_running;
    std::atomic<bool> m_businessRunning;
    std::thread m_thread;
    std::function<void()> m_onStart;
    std::function<void()> m_onStop;
};