#include "main.hpp"
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

Engine engine(80, 45);

int main()
{
	engine.load();
	while (!TCODConsole::isWindowClosed()) 
	{
		engine.update();
		engine.render();
		TCODConsole::flush();
	}
	engine.save();
	return 0;
}