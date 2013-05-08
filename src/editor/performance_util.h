#ifndef BASE_PERFORMANCE_UTIL_H_
#define BASE_PERFORMANCE_UTIL_H_
#pragma once

namespace base {

// Run the given function and print the time it took.
void TimeIt(void (*f)(), const char* desc);

} // namespace base

#endif // BASE_PERFORMANCE_UTIL_H_
