#pragma once

#include "Engine/Log.h"

//--- MACROS! 
#define LOG_CRITICAL(...)	Log::getInstance().Logger()->critical(__VA_ARGS__);
#define LOG_ERROR(...)		Log::getInstance().Logger()->error(__VA_ARGS__);
#define LOG_WARNING(...)	Log::getInstance().Logger()->warn(__VA_ARGS__);
#define LOG_INFO(...)		Log::getInstance().Logger()->info(__VA_ARGS__);
#define LOG_DEBUG(...)		Log::getInstance().Logger()->debug(__VA_ARGS__);
