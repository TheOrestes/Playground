
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"

#include "Application.h"

int main(int argc, char** argv)
{
	Application mainApp("Vulkan Playground", 1280, 800);
	mainApp.Run();

	return 0;
}