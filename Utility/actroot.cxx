#include "ActOptions.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        ActRoot::Options opts {argc, argv};
        opts.Print();
        opts.GetProjectDir();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << '\n';
        return 1;
    }
    return 0;
}
