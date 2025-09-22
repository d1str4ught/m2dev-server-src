#include "log.h"

#include <cstdarg>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "syslog_rotate_sink.h"

constexpr size_t LOGGER_QUEUE_SIZE = (1 << 14);
constexpr size_t LOGGER_NUM_THREADS = 1;

static std::shared_ptr<spdlog::logger> g_syslog;
static std::shared_ptr<spdlog::logger> g_syserr;

static bool g_bLogInitialized = false;

void log_init()
{
	if (g_bLogInitialized)
		return;

	spdlog::init_thread_pool(LOGGER_QUEUE_SIZE, LOGGER_NUM_THREADS);

	auto syslog_sink = std::make_shared<syslog_rotate_sink>();
	syslog_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

	g_syslog = std::make_shared<spdlog::async_logger>(
		"syslog",
		syslog_sink,
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::block);

	spdlog::register_logger(g_syslog);


	auto syserr_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("syserr.log", true);
	syserr_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%!()] %v");

	g_syserr = std::make_shared<spdlog::async_logger>(
		"syserr",
		syserr_sink,
		spdlog::thread_pool(),
		spdlog::async_overflow_policy::block);

	spdlog::register_logger(g_syserr);

#ifdef _DEBUG
	g_syslog->set_level(spdlog::level::debug);
#else
	g_syslog->set_level(spdlog::level::info);
#endif

	spdlog::flush_every(std::chrono::seconds(1));

	std::atexit([]() { log_destroy(); });

	g_bLogInitialized = true;
}

void log_destroy()
{
	if (!g_bLogInitialized)
		return;

	spdlog::shutdown();
	g_bLogInitialized = false;
}

void _sys_err(std::string_view str, const std::source_location& src_loc)
{
	spdlog::source_loc loc;
	loc.funcname = src_loc.function_name();
	loc.line = src_loc.line();
	loc.filename = src_loc.file_name();

	g_syserr->log(loc, spdlog::level::err, str);
}

void _sys_log(int level, std::string_view str)
{
	spdlog::level::level_enum lvl = spdlog::level::info;
	switch (level)
	{
		case 1:		lvl = spdlog::level::debug; break;
		case 2:		lvl = spdlog::level::trace; break;
		case 3:		lvl = spdlog::level::trace; break;
		default:	lvl = spdlog::level::info; break;
	}

	g_syslog->log(lvl, str);
}

std::string_view _format(std::string_view fmt, ...)
{
	constexpr int BUFFER_SIZE = 4096;
	thread_local char buffer[BUFFER_SIZE + 1];
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(buffer, BUFFER_SIZE, fmt.data(), args);
	va_end(args);
	return { buffer, (std::string_view::size_type) std::clamp(len, 0, BUFFER_SIZE) };
}
