// check.h - Mini-utilidad de tests sin dependencias.
#pragma once

#include <cstdlib>
#include <iostream>

#define CHECK(cond)                                                                        \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK fallido: " #cond << "\n"; \
            std::exit(1);                                                                  \
        }                                                                                  \
    } while (0)

#define CHECK_THROWS(expr, ExceptionType)            \
    do {                                             \
        bool thrown_ = false;                        \
        try {                                        \
            expr;                                    \
        } catch (const ExceptionType&) {             \
            thrown_ = true;                          \
        }                                            \
        CHECK(thrown_);                              \
    } while (0)
