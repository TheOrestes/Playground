
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "Application.h"

int main(int argc, char** argv)
{
	LOG_CRITICAL("Critical Log");
	LOG_ERROR("Error Log");
	LOG_DEBUG("Debug Log");
	LOG_INFO("Info Log");
	LOG_WARNING("Warning Log");

	Application mainApp("Vulkan Playground", 1280, 800);
	mainApp.Run();

	return 0;
}