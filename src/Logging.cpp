#include "pch.h"

#include "Config.h"
#include "Logging.h"

#include <spdlog/spdlog.h>

namespace Logging
{
	void Configure()
	{
		const auto config = Config::Current();
		const auto level = config->debugLogging ? spdlog::level::debug : spdlog::level::info;
		const auto logger = spdlog::default_logger();
		logger->set_level(level);
		logger->flush_on(level);
	}
}
