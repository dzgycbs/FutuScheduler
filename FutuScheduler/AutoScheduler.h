// AutoScheduler.h
#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <string>
#include <fstream>

class AutoScheduler {
public:
	enum class Action {
		START,
		STOP,
		NONE
	};

	struct ScheduleRule {
		int hour;
		int minute;
		Action action;
		std::vector<int> weekdays;
		std::string desc;
	};

	AutoScheduler();
	~AutoScheduler();

	AutoScheduler(const AutoScheduler&) = delete;
	AutoScheduler& operator=(const AutoScheduler&) = delete;

	void addRule(const ScheduleRule& rule);
	void setOnStartBusiness(std::function<void()> callback);
	void setOnStopBusiness(std::function<void()> callback);
	void start();
	void stop();
	bool isBusinessRunning() const;

	// 状态持久化
	void setStateFile(const std::string& path);

	// 立即同步业务状态（启动时调用）
	void syncBusinessState();

private:
	void run();
	void checkAndExecute();
	bool matchRule(const ScheduleRule& rule, const std::tm& tm) const;
	bool isInBusinessHours(const std::tm& tm) const;

	// 【保留】查找最近的规则
	const ScheduleRule* findNearestRule(const std::tm& tm) const;

	// 【新增】格式化星期
	std::string formatWeekdays(const std::vector<int>& weekdays) const;

	// 状态持久化
	void saveState() const;
	bool loadState();

private:
	std::vector<ScheduleRule> m_rules;
	std::atomic<bool> m_running;
	std::atomic<bool> m_businessRunning;
	std::thread m_thread;
	std::function<void()> m_onStart;
	std::function<void()> m_onStop;
	std::string m_stateFile;
};