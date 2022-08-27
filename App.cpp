#include "App.h"
#include <iomanip>

App::App()
	:
	wnd(800, 600, "Window")
{}

int App::Go()
{
	MSG msg;
	bool gResult;
	while (true)
	{
		// process all messages pending, but to not block for new messages.
		if (const auto ecode = Window::ProcessMessages())
		{
			// if return optional has value, it means we are returning so we must return the exit code.
			return *ecode;
		}
		DoFrame();
	} 

	// Check if GetMessage fails.
	if (gResult == -1)
	{
		throw CHWND_LAST_EXCEPT();
	}

	// wParam here is the value passed to PostQuitMessage.
	return msg.wParam;
}

void App::DoFrame()
{
	const float c = sin(timer.Peek()) / 2.0f + 0.5f;
	wnd.Gfx().ClearBuffer(1.0f, 1.0f, 1.0f);
	wnd.Gfx().DrawTestTriangle();
	wnd.Gfx().EndFrame();
}