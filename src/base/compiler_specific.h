#ifndef BASE_COMPILER_SPECIFIC_H_
#define BASE_COMPILER_SPECIFIC_H_
#pragma once

#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__clang__)
#define COMPILER_CLANG 1
#endif

// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(COMPILER_MSVC)
#define OVERRIDE override
#elif defined(COMPILER_CLANG)
#define OVERRIDE override
#else
#define OVERRIDE
#endif

#endif // BASE_COMPILER_SPECIFIC_H_
