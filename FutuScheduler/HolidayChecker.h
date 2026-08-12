// HolidayChecker.h
#pragma once

#include <string>
#include <unordered_set>
#include <fstream>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp>

class HolidayChecker {
public:
    static HolidayChecker& getInstance() {
        static HolidayChecker instance;
        return instance;
    }

    HolidayChecker(const HolidayChecker&) = delete;
    HolidayChecker& operator=(const HolidayChecker&) = delete;

    // ===== 加载配置 =====
    // 从 JSON 文件加载节假日列表
    bool loadFromFile(const std::string& configFile);

    // ===== 判断接口 =====
    // 判断某天是否为节假日（休市）
    bool isHoliday(const std::tm& tm) const;
    bool isHoliday(const std::string& dateStr) const;
    bool isTodayHoliday() const;

    // ===== 动态修改 =====
    void addHoliday(const std::string& date);
    void removeHoliday(const std::string& date);
    void clear();

    // ===== 查询接口 =====
    const std::unordered_set<std::string>& getHolidays() const { return m_holidays; }
    bool isLoaded() const { return m_loaded; }

    // ===== 调试接口 =====
    void dump() const;

private:
    HolidayChecker() : m_loaded(false) {}

    std::string tmToDateStr(const std::tm& tm) const;
    std::tm getTodayTm() const;
    bool parseDate(const std::string& dateStr, std::tm& tm) const;
    bool isWeekend(const std::tm& tm) const;

private:
    std::unordered_set<std::string> m_holidays;
    mutable std::mutex m_mutex;
    bool m_loaded;
};