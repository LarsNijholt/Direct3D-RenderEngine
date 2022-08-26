#include "Window.h"
#include "resource.h"
#include <sstream>


Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0));
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = static_cast<HICON>(LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0));
	RegisterClassEx(&wc);
}

Window::WindowClass::~WindowClass() { UnregisterClass(wndClassName, GetInstance()); }

const char* Window::WindowClass::GetName() noexcept { return wndClassName; }

HINSTANCE Window::WindowClass::GetInstance() noexcept { return wndClass.hInst; }

// Window Stuff.
Window::Window(int width, int height, const char* name)
	:
	width(width),
	height(height)
	
{
	// calculate window size based on desired client region size
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	if (!AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE))
	{
		throw CHWND_LAST_EXCEPT();
	}

	// create window & get hWnd.
	hWnd = CreateWindow(
		WindowClass::GetName(), name, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	);

	// Check for error
	if (hWnd == nullptr)
	{
		throw CHWND_LAST_EXCEPT();
	}

	// Show the window.
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	// Create graphics object.
	pGfx = std::make_unique<Graphics>(hWnd);
}

Window::~Window() { DestroyWindow(hWnd); }

void Window::SetTitle(const std::string title)
{
	if (!SetWindowText(hWnd, title.c_str()))
	{
		throw CHWND_LAST_EXCEPT();
	}
}

std::optional<int> Window::ProcessMessages() noexcept
{
	MSG msg;
	// While queue has messages, remove and dispatch them (but do not block on empty queue).
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		// Check for quit becuase peekmessage does not signal this via return.
		if (msg.message == WM_QUIT)
		{
			// Return optional wrapping in (arg to PostQuitMessage is in wParam.
			return msg.wParam;
		}

		// TranslateMessage will post auxilliary WM_CHAR messages from key msgs.
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Return empty optional when not quitting app.
	return {};
}

Graphics& Window::Gfx()
{
	if (!pGfx)
	{
		throw CHWND_NOGFX_EXCEPT();
	}
	return* pGfx;
}

LRESULT WINAPI Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Use create paramater passed in from CreateWindow() to store window class pointer.
	if (msg == WM_NCCREATE)
	{
		// Extract ptr to window class from creation data.
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		// set WinApi-managed user data to store ptr to window class.
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		// Set message proc to normal (non-setup) handler now that setup is finished.
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
		// Forward message to window class handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
	// If we get a message before the WM_NCCreate message, handle with default handler.
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WINAPI Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	// Retrieve ptr to window class.
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	// Forward message to window class handler.
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg)
	{
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		// clear keystae when window loses focus.
		case WM_KILLFOCUS:
		{
			kbd.ClearState();
			break;
		}
		// Keyboard messages.
		case WM_KEYDOWN:
		{
		case WM_SYSKEYDOWN:
			if (!(lParam & 0x40000000) || kbd.AutorepeatIsEnabled()) // Filter autorepeat.
			{
				kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
			}
			break;
		}
		case WM_KEYUP:
		{
			kbd.OnKeyReleased(static_cast<unsigned char>(wParam));
			break;
		}
		case WM_CHAR:
			kbd.OnChar(static_cast<unsigned char>(wParam));
			break;
			// Mouse messages.
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			// in client region -> log move, and log enter + capture mouse.
			if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
			{
				mouse.OnMouseMove(pt.x, pt.y);
				if (!mouse.IsInWindow())
				{
					SetCapture(hWnd);
					mouse.OnMouseEnter();
				}

			}
			// Not in client -> log move / maintain capture if buttown down
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					mouse.OnMouseMove(pt.x, pt.y);
				}
				// Button up -> release capture / log event for leaving.
				else
				{
					ReleaseCapture();
					mouse.OnMouseLeave();
				}
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnLeftPressed(pt.x, pt.y);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnRightPressed(pt.x, pt.y);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			mouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}
	}


	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Window Exception
std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
{
	char* pMsgBuf = nullptr;
	// Windows will allocate memory for error string and make our pointer point to it.
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pMsgBuf), 0, nullptr
	);
	// 0 string length returned indicates a failure
	if (nMsgLen == 0)
	{
		return "Unidentified error code";
	}
	// Copy error string from windows-allocated buffer to std::string
	std::string errorString = pMsgBuf;
	// free windows buffer.
	LocalFree(pMsgBuf);
	return errorString;
}

Window::HrException::HrException(int line, const char* file, HRESULT hr) noexcept
	:
	Exception(line, file),
	hr(hr)
{}

const char* Window::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl
		<< GetOriginString();
	whatbuffer = oss.str();
	return whatbuffer.c_str();
}

const char* Window::HrException::GetType() const noexcept
{
	return "Window Exception Thrown!";
}

HRESULT Window::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string Window::HrException::GetErrorDescription() const noexcept
{
	return Exception::TranslateErrorCode(hr);
}

const char* Window::NoGfxException::GetType() const noexcept
{
	return "Window Exception Thrown! [No Graphics]";
}