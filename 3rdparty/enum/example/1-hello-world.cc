// This file was generated automatically.

// Hello, World!
//
// Download enum.h, then compile this program:

#include <iostream>
#include "enum.h"
#include <Logging.h>

BETTER_ENUM(Word, int, Hello, World)

int main()
{
    CLOG_INFO << Word::Hello;
    CLOG_INFO << Word::World;
    std::cout << (+Word::Hello)._to_string() << ", " << (+Word::World)._to_string() << "!" << std::endl;

    return 0;
}

// Run it, and you should see the output "Hello, World!"
//
// Congratulations, you have just created your first Better Enum!
