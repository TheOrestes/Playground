
#include "Log.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Log::Log()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	m_pLogger = spdlog::default_logger_raw();
	m_pLogger->set_level(spdlog::level::trace);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Log::~Log()
{

}
