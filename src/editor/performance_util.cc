#include "editor/performance_util.h"
#include <iostream>
#include "boost/chrono/chrono.hpp"

namespace base {

void TimeIt(void (*f)(), const char* desc) {
  boost::chrono::system_clock::time_point start =
    boost::chrono::system_clock::now();

  f();

  boost::chrono::duration<double> sec =
    boost::chrono::system_clock::now() - start;
  std::cout << desc << ": " << sec.count() << "s\n";
}

} // namespace base
