#pragma once
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

class syslog_rotate_sink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    syslog_rotate_sink() {
        open_stream();

        auto now = spdlog::log_clock::now();
        m_current_day = day_string(now_tm(now));
        m_rotation_tp = next_rotation_tp(now);
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        auto time = msg.time;
        if (time >= m_rotation_tp) {
            rotate(time);
        }

        spdlog::memory_buf_t formatted;
        base_sink<std::mutex>::formatter_->format(msg, formatted);
        m_file.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
    }

    void flush_() override {
        m_file.flush();
    }

protected:
    void open_stream() {
        m_file.open("syslog.log", std::ios::out | std::ios::app | std::ios::binary);
    }

    void rotate(const spdlog::log_clock::time_point& now_tp) {
        m_file.close();

        std::error_code ec;
        const auto archived = fmt::format("log/syslog_{}.log", m_current_day);
        std::filesystem::rename("syslog.log", archived, ec);
        if (ec) {
            std::filesystem::copy_file(
                "syslog.log",
                archived,
                std::filesystem::copy_options::overwrite_existing,
                ec
            );
            std::filesystem::remove("syslog.log");
        }

        try {
            purge_old_(now_tp, 7);
        }
        catch (...) {
        }

        m_current_day = day_string(now_tm(now_tp));
        m_rotation_tp = next_rotation_tp(now_tp);
        open_stream();
    }

private:
    static std::tm now_tm(spdlog::log_clock::time_point tp) {
        std::time_t t = spdlog::log_clock::to_time_t(tp);
        return spdlog::details::os::localtime(t);
    }

    static std::string day_string(const std::tm& tm) {
        char buf[16];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
        return buf;
    }

    static spdlog::log_clock::time_point next_rotation_tp(spdlog::log_clock::time_point now) {
        std::tm tm = now_tm(now);
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        auto midnight = spdlog::log_clock::from_time_t(std::mktime(&tm));
        if (midnight > now) {
            return midnight;
        }
        return midnight + std::chrono::hours(24);
    }

    void purge_old_(const spdlog::log_clock::time_point& now_tp, int days) {
        auto cutoff_tp = now_tp - std::chrono::hours(24 * days);
        const std::string cutoff = day_string(now_tm(cutoff_tp));

        std::error_code ec;
        for (auto& entry : std::filesystem::directory_iterator("log", ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;

            const auto name = entry.path().filename().string();
            if (name.size() != std::strlen("syslog_YYYY-MM-DD.log")) continue;
            if (name.rfind("syslog_", 0) != 0) continue;

            const std::string date_part = name.substr(7, 10);
            if (date_part < cutoff) {
                std::filesystem::remove(entry.path(), ec);
            }
        }
    }

private:
    std::ofstream m_file;
    std::string   m_current_day;
    spdlog::log_clock::time_point m_rotation_tp;
};
