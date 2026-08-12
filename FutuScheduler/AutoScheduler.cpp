// AutoScheduler.cpp
#include "AutoScheduler.h"
#include "Logger.h"
#include "HolidayChecker.h"

// ===== 构造函数/析构函数 =====
AutoScheduler::AutoScheduler()
    : m_running(false)
    , m_businessRunning(false) {
}

AutoScheduler::~AutoScheduler() {
    stop();
}

// ===== 公共接口 =====
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

void AutoScheduler::setStateFile(const std::string& path) {
    m_stateFile = path;
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

    // 【关键】启动线程前先同步一次状态
    syncBusinessState();

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

// ===== 状态同步 =====
void AutoScheduler::syncBusinessState() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);

    LOG_INFO("Syncing business state at {:02d}:{:02d} weekday={}",
        tm.tm_hour, tm.tm_min, tm.tm_wday);

    // 加载上次保存的状态
    bool lastState = false;
    if (loadState()) {
        lastState = m_businessRunning;
        LOG_DEBUG("Loaded last state: {}", lastState ? "running" : "stopped");
    }

    // 判断当前是否应该运行
    bool shouldStart = isInBusinessHours(tm);
    bool isRunning = m_businessRunning;

    if (shouldStart && !isRunning) {
        LOG_INFO("Business should be running, starting now...");
        if (m_onStart) {
            m_onStart();
            m_businessRunning = true;
            saveState();
            LOG_INFO("Business started by state sync");
        }
    }
    else if (!shouldStart && isRunning) {
        LOG_INFO("Business should be stopped, stopping now...");
        if (m_onStop) {
            m_onStop();
            m_businessRunning = false;
            saveState();
            LOG_INFO("Business stopped by state sync");
        }
    }
    else {
        LOG_INFO("Business state unchanged: {}", isRunning ? "running" : "stopped");
    }

    // 【使用 findNearestRule】打印下次调度时间
    const ScheduleRule* nearest = findNearestRule(tm);
    if (nearest) {
        std::string wdStr = nearest->weekdays.empty() ? "all" : formatWeekdays(nearest->weekdays);
        LOG_INFO("Next scheduled action: [{}] at {:02d}:{:02d} ({})",
            nearest->action == Action::START ? "START" : "STOP",
            nearest->hour, nearest->minute, wdStr);
    }
    else {
        LOG_WARN("No future schedule found!");
    }
}

// ===== 核心调度循环 =====
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

        // 每分钟检查一次
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

        LOG_INFO("Scheduler executing rule: {} at {:02d}:{:02d}",
            rule.desc, rule.hour, rule.minute);

        if (rule.action == Action::START && !m_businessRunning) {
            if (m_onStart) {
                m_onStart();
                m_businessRunning = true;
                saveState();
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
                saveState();
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

// ===== 匹配判断 =====
bool AutoScheduler::matchRule(const ScheduleRule& rule, const std::tm& tm) const {
    // 检查小时和分钟
    if (tm.tm_hour != rule.hour || tm.tm_min != rule.minute) {
        return false;
    }

    // 检查星期几
    int currentWeekday = tm.tm_wday == 0 ? 7 : tm.tm_wday;

    if (rule.weekdays.empty()) {
        return true;
    }

    for (int wd : rule.weekdays) {
        if (wd == currentWeekday) {
            return true;
        }
    }

    return false;
}

bool AutoScheduler::isInBusinessHours(const std::tm& tm) const {

    if (HolidayChecker::getInstance().isHoliday(tm)) {
        LOG_DEBUG("Today is holiday, business stopped");
        return false;
    }


    int currentWeekday = tm.tm_wday == 0 ? 7 : tm.tm_wday;
    int currentMinutes = tm.tm_hour * 60 + tm.tm_min;

    std::vector<const ScheduleRule*> todayStarts;
    std::vector<const ScheduleRule*> todayStops;

    for (const auto& rule : m_rules) {
        if (!rule.weekdays.empty()) {
            bool match = false;
            for (int wd : rule.weekdays) {
                if (wd == currentWeekday) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;
        }

        if (rule.action == Action::START) {
            todayStarts.push_back(&rule);
        }
        else if (rule.action == Action::STOP) {
            todayStops.push_back(&rule);
        }
    }

    if (todayStarts.empty() || todayStops.empty()) {
        return false;
    }

    // 构建时间段
    struct TimeSegment {
        int start;
        int end;
    };
    std::vector<TimeSegment> segments;

    for (const auto* startRule : todayStarts) {
        int startMin = startRule->hour * 60 + startRule->minute;

        const ScheduleRule* matchedStop = nullptr;
        int minDiff = 24 * 60;

        for (const auto* stopRule : todayStops) {
            int stopMin = stopRule->hour * 60 + stopRule->minute;
            int diff = stopMin - startMin;
            if (diff <= 0) diff += 24 * 60;
            if (diff < minDiff) {
                minDiff = diff;
                matchedStop = stopRule;
            }
        }

        if (matchedStop) {
            int endMin = matchedStop->hour * 60 + matchedStop->minute;
            if (endMin <= startMin) {
                endMin += 24 * 60;
            }
            segments.push_back({ startMin, endMin });
        }
    }

    int currentTotalMinutes = currentMinutes;

    for (const auto& seg : segments) {
        if (currentTotalMinutes >= seg.start && currentTotalMinutes < seg.end) {
            return true;
        }
        if (seg.end > 24 * 60) {
            int adjustedCurrent = currentTotalMinutes + 24 * 60;
            if (adjustedCurrent >= seg.start && adjustedCurrent < seg.end) {
                return true;
            }
        }
    }

    return false;
}

// ===== findNearestRule 实现 =====
const AutoScheduler::ScheduleRule* AutoScheduler::findNearestRule(const std::tm& tm) const {
    const ScheduleRule* nearest = nullptr;
    int minDiff = 7 * 24 * 60 + 1;

    int currentWeekday = tm.tm_wday == 0 ? 7 : tm.tm_wday;
    int currentMinutes = tm.tm_hour * 60 + tm.tm_min;

    for (const auto& rule : m_rules) {
        int ruleMinutes = rule.hour * 60 + rule.minute;
        int diff = ruleMinutes - currentMinutes;
        int wdDiff = 0;

        if (diff <= 0) {
            diff += 24 * 60;
            wdDiff = 1;
        }

        if (!rule.weekdays.empty()) {
            bool found = false;
            for (int dayOffset = 0; dayOffset <= 7; ++dayOffset) {
                int checkWd = currentWeekday + wdDiff + dayOffset;
                while (checkWd > 7) checkWd -= 7;

                for (int wd : rule.weekdays) {
                    if (wd == checkWd) {
                        found = true;
                        diff = ruleMinutes - currentMinutes + (wdDiff + dayOffset) * 24 * 60;
                        if (diff <= 0) diff += 24 * 60;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found) continue;
        }
        else {
            diff = ruleMinutes - currentMinutes;
            if (diff <= 0) diff += 24 * 60;
        }

        if (diff < minDiff) {
            minDiff = diff;
            nearest = &rule;
        }
    }

    return nearest;
}

// ===== 辅助函数 =====
std::string AutoScheduler::formatWeekdays(const std::vector<int>& weekdays) const {
    if (weekdays.empty()) return "everyday";

    static const char* wdNames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    std::string result;
    for (size_t i = 0; i < weekdays.size(); ++i) {
        if (i > 0) result += ",";
        int idx = weekdays[i] - 1;
        if (idx >= 0 && idx < 7) {
            result += wdNames[idx];
        }
    }
    return result;
}

// ===== 状态持久化 =====
void AutoScheduler::saveState() const {
    if (m_stateFile.empty()) return;

    try {
        std::ofstream file(m_stateFile);
        if (file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time_t);

            file << "# AutoScheduler State File\n";
            file << "timestamp=" << time_t << "\n";
            file << "business_running=" << (m_businessRunning ? "1" : "0") << "\n";
            file << "date=" << (tm.tm_year + 1900) << "-"
                << (tm.tm_mon + 1) << "-" << tm.tm_mday << "\n";
            file << "time=" << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << "\n";
            file.close();
            LOG_DEBUG("State saved to {}", m_stateFile);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to save state: {}", e.what());
    }
}

bool AutoScheduler::loadState() {
    if (m_stateFile.empty()) return false;

    try {
        std::ifstream file(m_stateFile);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            if (line.find("business_running=") == 0) {
                std::string value = line.substr(17);
                m_businessRunning = (value == "1");
                file.close();
                return true;
            }
        }
        file.close();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to load state: {}", e.what());
    }

    return false;
}