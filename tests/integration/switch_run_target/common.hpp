#ifndef SWITCH_RUN_TARGET_HEADER
#define SWITCH_RUN_TARGET_HEADER

#include <string_view>

// Macros for consistent, constexpr names of marker files across test processes

#define PROC_FILES(x)                                               \
    constexpr std::string_view x##_started = "proc_" #x "_started"; \
    constexpr std::string_view x##_terminating = "proc_" #x "_terminating";

PROC_FILES(a)
PROC_FILES(b)
PROC_FILES(d)
PROC_FILES(e)

#undef PROC_FILES

#endif
