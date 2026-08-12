// HolidayChecker.cpp
#include "HolidayChecker.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>

bool HolidayChecker::loadFromFile(const std::string& configFile) {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        std::ifstream file(configFile);
        if (!file.is_open()) {
            LOG_WARN("Holiday config file not found: {}, using empty config", configFile);
            m_loaded = true;
            return false;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        m_holidays.clear();

        if (j.contains("holidays") && j["holidays"].is_array()) {
            for (const auto& item : j["holidays"]) {
                std::string date = item.get<std::string>();
                if (date.length() == 10) {  // YYYY-MM-DD
                    m_holidays.insert(date);
                }
            }
        }

        m_loaded = true;
        LOG_INFO("Loaded {} holidays from {}", m_holidays.size(), configFile);

        // 打印前几个供确认
        if (!m_holidays.empty()) {
            std::string sample;
            int count = 0;
            for (const auto& h : m_holidays) {
                if (count++ < 5) {
                    if (!sample.empty()) sample += ", ";
                    sample += h;
                }
            }
            LOG_DEBUG("Holidays sample: {}", sample);
        }

        return true;

    }
    catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Failed to parse holiday JSON: {}", e.what());
        m_loaded = true;
        return false;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to load holiday config: {}", e.what());
        m_loaded = true;
        return false;
    }
}

bool HolidayChecker::isHoliday(const std::tm& tm) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string dateStr = tmToDateStr(tm);

    // 在节假日列表中 → 休市
    if (m_holidays.find(dateStr) != m_holidays.end()) {
        return true;
    }

    // 不在节假日列表 → 开市（周末调休补班也开不了，因为交易所不开）
    return false;
}

bool HolidayChecker::isHoliday(const std::string& dateStr) const {
    std::tm tm = {};
    if (!parseDate(dateStr, tm)) {
        return false;
    }
    return isHoliday(tm);
}

bool HolidayChecker::isTodayHoliday() const {
    std::tm tm = getTodayTm();
    return isHoliday(tm);
}

void HolidayChecker::addHoliday(const std::string& date) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_holidays.insert(date);
    LOG_DEBUG("Added holiday: {}", date);
}

void HolidayChecker::removeHoliday(const std::string& date) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_holidays.erase(date);
    LOG_DEBUG("Removed holiday: {}", date);
}

void HolidayChecker::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_holidays.clear();
    m_loaded = false;
    LOG_DEBUG("Holiday checker cleared");
}

void HolidayChecker::dump() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("=== HolidayChecker Dump ===");
    LOG_INFO("Loaded: {}", m_loaded);
    LOG_INFO("Holidays ({}):", m_holidays.size());
    for (const auto& h : m_holidays) {
        LOG_INFO("  - {}", h);
    }
    LOG_INFO("=== End Dump ===");
}

// ===== 私有辅助函数 =====

std::string HolidayChecker::tmToDateStr(const std::tm& tm) const {
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return std::string(buf);
}

std::tm HolidayChecker::getTodayTm() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    return tm;
}

bool HolidayChecker::parseDate(const std::string& dateStr, std::tm& tm) const {
    if (dateStr.length() != 10) return false;
    if (dateStr[4] != '-' || dateStr[7] != '-') return false;

    try {
        tm.tm_year = std::stoi(dateStr.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(dateStr.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(dateStr.substr(8, 2));
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        std::mktime(&tm);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool HolidayChecker::isWeekend(const std::tm& tm) const {
    int wday = tm.tm_wday;
    return (wday == 0 || wday == 6);
}