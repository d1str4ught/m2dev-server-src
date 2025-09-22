#pragma once
#include <string_view>
#include <source_location>

void log_init();
void log_destroy();

void _sys_err(std::string_view str, const std::source_location& src_loc = std::source_location::current());
void _sys_log(int level, std::string_view str);

std::string_view _format(std::string_view fmt, ...);

#define sys_err(fmt, ...) _sys_err(_format(fmt __VA_OPT__(, __VA_ARGS__)))
#define sys_log(level, fmt, ...) _sys_log(level, _format(fmt __VA_OPT__(, __VA_ARGS__)))
