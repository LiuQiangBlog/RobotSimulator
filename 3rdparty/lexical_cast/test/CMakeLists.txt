cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

find_package(Threads REQUIRED)

set(TESTS
        lexical_cast/array
        lexical_cast/bool
        lexical_cast/char
        lexical_cast/chrono
        lexical_cast/map
        lexical_cast/numbers
        lexical_cast/optional
        lexical_cast/pair
        lexical_cast/set
        lexical_cast/string
        lexical_cast/tuple
        lexical_cast/util
        lexical_cast/vector
)

set(LIBRARIES
        lexical_cast
        stream::stream
        gtest_main
        Threads::Threads
)

configure_tests("core" "${LIBRARIES}" ${TESTS})
