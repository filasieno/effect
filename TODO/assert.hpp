#pragma once

// #include <cstdio>
// #include <cstdlib>
// #include <format>
// #include <source_location>

// #ifdef NDEBUG
// #define assertf(expr, ...) ((void)0)
// #else
// #define assertf(expr, ...) \
// 	do { \
// 		if (!(expr)) { \
// 			std::ios_base::sync_with_stdio(true); \
// 			std::fflush(nullptr); \
// 			std::fprintf(stderr, "\nAssertion failed: %s:%d", __FILE__, __LINE__); \
// 			std::fprintf(stderr, ": " #expr ": " __VA_ARGS__); \
// 			std::fprintf(stderr, "\n"); \
// 			std::fflush(nullptr); \
// 			std::abort(); \
// 		} \
// 	} while (0)
// #endif

// #ifdef NDEBUG
// #define ::ak::_Assert(expr, ...) ((void)0)
// #else
// #define ::ak::_Assert(expr, fmt, ...) ((void)0)
// namespace ak {

// 	template<typename... Args>
// 	inline void _Assert(bool expr, std::string_view fmt, Args&&... args, const std::source_location& loc = std::source_location::current()) {
// 		if (!expr && !std::is_constant_evaluated()) {
// 			std::ios_base::sync_with_stdio(true);
// 			std::fflush(nullptr);
// 			std::print(std::format("{}:{}: assertion failed: {}", loc.file_name(), loc.line(), std::vformat(fmt, std::make_format_args(args...))));
// 			std::fflush(nullptr);
// 			std::abort();
// 			//throw std::runtime_error(std::format("{}:{}: assertion failed: {}", loc.file_name(), loc.line(), std::vformat(fmt, std::make_format_args(args...))));
	
// 		}
// 	}

// 	inline void _Assert(bool expr, const std::source_location& loc = std::source_location::current()) {
// 		if (!expr && !std::is_constant_evaluated()) {
// 			std::ios_base::sync_with_stdio(true);
// 			std::fflush(nullptr);
// 			std::print(std::format("{}:{}: assertion failed: {}", loc.file_name(), loc.line(), std::vformat(fmt, std::make_format_args(args...))));
// 			std::fflush(nullptr);
// 			std::abort();	
// 		}
// 	}
// }

// #endif // NDEBUG

