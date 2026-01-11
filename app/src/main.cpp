#include "core/application.hpp"
#include "core/utils/logger/logger.hpp"

using namespace c2l;

[[noreturn]] int main()
{
	try
	{
		Application app{};

		app.run();
	}
	catch(const std::exception& e)
	{
		LOG_FATAL("Unhandled exception: {}", e.what());
	}
}