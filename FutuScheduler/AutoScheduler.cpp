// AutoScheduler.cpp
#include "AutoScheduler.h"
#include "Logger.h"  // 使用你之前封装的日志类

AutoScheduler::AutoScheduler()
    : m_running(false)
    , m_businessRunning(false) {
}

AutoScheduler::~AutoScheduler() {
    stop();
}

void AutoScheduler::addRule(const ScheduleRule& rule) {
    m_rules.push_back(rule);
    LOG_DEBUG("Added schedule rule: {} at {:02d}:{:02d}",
        rule.desc, rule.hour, rule.minute);
}

void AutoScheduler::setOnStartBusiness(std::function<void()> callback) {
    m_onStart = callback;
}

void AutoScheduler::setOnStopBusiness(std::function<void()> callback) {
    m_onStop = callback;
}

void AutoScheduler::start() {
    if (m_running) {
        LOG_WARN("AutoScheduler already running");
        return;
    }

    if (m_rules.empty()) {
        LOG_WARN("AutoScheduler started with no rules");
    }

    m_running = true;
    m_thread = std::thread(&AutoScheduler::run, this);
    LOG_INFO("AutoScheduler started with {} rules", m_rules.size());
}

void AutoScheduler::stop() {
    if (!m_running) {
        return;
    }

    LOG_INFO("AutoScheduler stopping...");
    m_running = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }

    LOG_INFO("AutoScheduler stopped");
}

bool AutoScheduler::isBusinessRunning() const {
    return m_businessRunning;
}

void AutoScheduler::run() {
    LOG_DEBUG("AutoScheduler thread started");

    while (m_running) {
        try {
            checkAndExecute();
        }
        catch (const std::exception& e) {
            LOG_ERROR("Scheduler check error: {}", e.what());
        }
        catch (...) {
            LOG_ERROR("Scheduler check error: unknown exception");
        }

        // 每分钟检查一次（可根据需要调整间隔）
        for (int i = 0; i < 60 && m_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    LOG_DEBUG("AutoScheduler thread exited");
}

void AutoScheduler::checkAndExecute() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);

    for (const auto& rule : m_rules) {
        if (!matchRule(rule, tm)) {
            continue;
        }

        // 时间匹配，执行动作
        LOG_INFO("Scheduler executing rule: {} at {:02d}:{:02d}",
            rule.desc, rule.hour, rule.minute);

        if (rule.action == Action::START && !m_businessRunning) {
            if (m_onStart) {
                m_onStart();
                m_businessRunning = true;
                LOG_INFO("Business started by scheduler");
            }
            else {
                LOG_WARN("Start callback not set");
            }
        }
        else if (rule.action == Action::STOP && m_businessRunning) {
            if (m_onStop) {
                m_onStop();
                m_businessRunning = false;
                LOG_INFO("Business stopped by scheduler");
            }
            else {
                LOG_WARN("Stop callback not set");
            }
        }
        else if (rule.action == Action::START && m_businessRunning) {
            LOG_DEBUG("Business already running, ignoring START rule: {}", rule.desc);
        }
        else if (rule.action == Action::STOP && !m_businessRunning) {
            LOG_DEBUG("Business already stopped, ignoring STOP rule: {}", rule.desc);
        }
    }
}

bool AutoScheduler::matchRule(const ScheduleRule& rule, const std::tm& tm) const {
    // 检查小时和分钟
    if (tm.tm_hour != rule.hour || tm.tm_min != rule.minute) {
        return false;
    }

    // 检查星期几（tm_wday: 0=周日, 1=周一, ...）
    int currentWeekday = tm.tm_wday == 0 ? 7 : tm.tm_wday;  // 转为 1-7

    // 如果规则没有指定星期，则每天都匹配
    if (rule.weekdays.empty()) {
        return true;
    }

    // 检查当前星期是否在规则列表中
    for (int wd : rule.weekdays) {
        if (wd == currentWeekday) {
            return true;
        }
    }

    return false;
}