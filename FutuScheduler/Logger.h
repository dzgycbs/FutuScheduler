// Logger.h
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>



class Logger {
public:
	// 禁止拷贝和赋值
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	// 获取单例实例
	static Logger& getInstance() {
		static Logger instance;
		return instance;
	}

	// 初始化日志系统（需要在程序启动时调用一次）
	// @param log_dir: 日志文件目录，默认为 "logs"
	// @param log_filename: 日志文件名，默认为 "app"
	// @param console_level: 控制台输出级别，默认为 info
	// @param file_level: 文件输出级别，默认为 debug
	// @param rotate_hour: 日志滚动的小时，默认为 20
	// @param rotate_minute: 日志滚动的分钟，默认为 0
	void init(const std::string& log_dir = "logs",
		const std::string& log_filename = "app",
		spdlog::level::level_enum console_level = spdlog::level::info,
		spdlog::level::level_enum file_level = spdlog::level::debug,
		int rotate_hour = 20,
		int rotate_minute = 0) {

		if (m_initialized) {
			spdlog::warn("Logger already initialized, skipping...");
			return;
		}

		try {
			// 构建完整的文件路径
			std::string file_path = log_dir + "/" + log_filename + ".txt";

			// 创建控制台sink（彩色输出）
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(console_level);
			console_sink->set_pattern("%^[%H:%M:%S] [%l] %v%$");

			// 创建文件sink（每日滚动）
			auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
				file_path, rotate_hour, rotate_minute
				);
			file_sink->set_level(file_level);
			file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");

			// 组合sinks
			spdlog::sinks_init_list sinks = { console_sink, file_sink };
			m_logger = std::make_shared<spdlog::logger>("multi_sink", sinks);
			m_logger->set_level(spdlog::level::trace);  // 总开关设为最低级别
			m_logger->flush_on(file_level);  // 根据文件级别设置刷新

			// 设置为默认logger
			spdlog::set_default_logger(m_logger);

			m_initialized = true;
			m_log_dir = log_dir;
			m_log_filename = log_filename;

			spdlog::info("Logger initialized successfully. Log file: {}", file_path);
		}
		catch (const spdlog::spdlog_ex& ex) {
			// 如果初始化失败，回退到控制台输出
			spdlog::warn("Failed to initialize file logger: {}, falling back to console only", ex.what());
			auto console_logger = spdlog::stdout_color_mt("console");
			spdlog::set_default_logger(console_logger);
			m_initialized = true;
		}
	}

	// 获取logger指针（用于特殊场景）
	std::shared_ptr<spdlog::logger> getLogger() const {
		return m_logger;
	}

	// 设置日志级别（全局）
	void setLevel(spdlog::level::level_enum level) {
		if (m_logger) {
			m_logger->set_level(level);
		}
	}

	// 设置控制台级别
	void setConsoleLevel(spdlog::level::level_enum level) {
		if (m_logger) {
			auto sinks = m_logger->sinks();
			if (sinks.size() > 0) {
				sinks[0]->set_level(level);  // 第一个是console sink
			}
		}
	}

	// 设置文件级别
	void setFileLevel(spdlog::level::level_enum level) {
		if (m_logger) {
			auto sinks = m_logger->sinks();
			if (sinks.size() > 1) {
				sinks[1]->set_level(level);  // 第二个是file sink
				m_logger->flush_on(level);
			}
		}
	}

	// 手动刷新日志
	void flush() {
		if (m_logger) {
			m_logger->flush();
		}
	}

	// 关闭日志系统（程序退出前调用）
	void shutdown() {
		if (m_initialized) {
			spdlog::info("Shutting down logger...");
			spdlog::shutdown();
			m_initialized = false;
			m_logger.reset();
		}
	}

	// 检查是否已初始化
	bool isInitialized() const {
		return m_initialized;
	}

private:
	// 私有构造函数
	Logger() : m_initialized(false) {}

	// 析构函数
	~Logger() {
		shutdown();
	}

private:
	std::shared_ptr<spdlog::logger> m_logger;
	std::string m_log_dir;
	std::string m_log_filename;
	bool m_initialized;
};

// Logger.h 中的增强宏定义

// ===== 基础日志宏（支持可变参数） =====
#define LOG_TRACE(...)    spdlog::trace(__VA_ARGS__)
#define LOG_DEBUG(...)    spdlog::debug(__VA_ARGS__)
#define LOG_INFO(...)     spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)     spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)    spdlog::error(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

// ===== 带文件位置和行号的宏（支持可变参数） =====
#define LOG_TRACE_LOC(...)    spdlog::trace("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define LOG_DEBUG_LOC(...)    spdlog::debug("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define LOG_INFO_LOC(...)     spdlog::info("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define LOG_WARN_LOC(...)     spdlog::warn("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define LOG_ERROR_LOC(...)    spdlog::error("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define LOG_CRITICAL_LOC(...) spdlog::critical("[{}:{}] " + fmt::format(__VA_ARGS__), __FILE__, __LINE__)

// ===== 只在 Debug 模式下生效的日志宏（支持可变参数） =====
#ifdef _DEBUG
#define LOG_DEBUG_ONLY(...)   LOG_DEBUG(__VA_ARGS__)
#define LOG_DEBUG_LOC_ONLY(...) LOG_DEBUG_LOC(__VA_ARGS__)
#else
#define LOG_DEBUG_ONLY(...)   ((void)0)
#define LOG_DEBUG_LOC_ONLY(...) ((void)0)
#endif

// ===== 带条件判断的日志宏（支持可变参数） =====
#define LOG_IF(condition, level, ...) \
    do { \
        if (condition) { \
            spdlog::level(__VA_ARGS__); \
        } \
    } while(0)

// 使用示例：LOG_IF(error_code != 0, error, "Operation failed with code: {}", error_code)

// ===== 每 N 次才打印一次的日志宏（避免日志刷屏） =====
#define LOG_ONCE(level, ...) \
    do { \
        static bool __logged = false; \
        if (!__logged) { \
            __logged = true; \
            spdlog::level(__VA_ARGS__); \
        } \
    } while(0)

#define LOG_EVERY_N(level, N, ...) \
    do { \
        static int __counter = 0; \
        if (++__counter % N == 0) { \
            spdlog::level(__VA_ARGS__); \
        } \
    } while(0)

// 使用示例：LOG_EVERY_N(info, 100, "Processing {} items", counter);

// ===== 带时间统计的日志宏（方便性能分析） =====
#define LOG_WITH_TIMER(level, message, ...) \
    do { \
        auto __start = std::chrono::steady_clock::now(); \
        spdlog::level(message, __VA_ARGS__); \
        auto __end = std::chrono::steady_clock::now(); \
        auto __duration = std::chrono::duration_cast<std::chrono::milliseconds>(__end - __start); \
        spdlog::debug("{} took {} ms", message, __duration.count()); \
    } while(0)

// ===== 结构化日志（JSON格式，方便日志分析） =====
#define LOG_STRUCTURED(level, ...) \
    do { \
        std::string __json = "{" + fmt::format(__VA_ARGS__) + "}"; \
        spdlog::level(__json); \
    } while(0)

// 使用示例：LOG_STRUCTURED(info, "\"event\":\"user_login\", \"user\":\"{}\", \"ip\":\"{}\"", username, ip);