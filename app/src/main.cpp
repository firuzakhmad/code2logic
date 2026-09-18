#include "core/application.hpp"
#include "core/utils/logger/logger.hpp"
#include <exception>
#include <cstdlib>

using namespace c2l;

int main()
{
    try
    {
        Application app{};
        app.run();
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Unhandled exception: {}", e.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        LOG_FATAL("Unhandled non-standard exception");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}