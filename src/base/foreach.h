#ifndef BASE_FOREACH_H_
#define BASE_FOREACH_H_
#pragma once

#include "boost/foreach.hpp"

// Make BOOST_FOREACH prettier.
#ifndef foreach
#   define foreach BOOST_FOREACH
#endif
#ifndef reverse_foreach
#   define reverse_foreach BOOST_REVERSE_FOREACH
#endif

#endif // BASE_FOREACH_H_
