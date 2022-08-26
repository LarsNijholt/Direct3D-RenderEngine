#pragma once
#include "Window.h"
#include "Timer.h"

class App
{
public:
	App();
	// Master frame / message loop.
	int Go();
private:
	void DoFrame();
private:
	Window wnd;
	Timer timer;
};