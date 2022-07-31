#include <algorithm>
#include "stdafx.h"
#include "ER_Core.h"
#include "ER_CoreException.h"
#include "ER_Utility.h"

#include "imgui.h"
#include "imgui_impl_win32.h"

namespace Library
{
	ER_Core::ER_Core(ER_RHI* aRHI, HINSTANCE instance, const std::wstring& windowClass, const std::wstring& windowTitle, int showCommand,
		UINT width, UINT height, bool isFullscreen)
		: 
		mInstance(instance),
		mWindowClass(windowClass),
		mWindowTitle(windowTitle),
		mShowCommand(showCommand),
		mWindowHandle(),
		mWindow(),
		mScreenWidth(width),
		mScreenHeight(height),
		mIsFullscreen(isFullscreen),
		mCoreClock(),
		mCoreTime(),
		mCoreEngineComponents(),
		mServices(),
		mCPUProfiler(new ER_CPUProfiler()),
		mRHI(aRHI)
	{
	}

	ER_Core::~ER_Core()
	{
	}

	HINSTANCE ER_Core::Instance() const
	{
		return mInstance;
	}

	HWND ER_Core::WindowHandle() const
	{
		return mWindowHandle;
	}

	const WNDCLASSEX& ER_Core::Window() const
	{
		return mWindow;
	}

	const std::wstring& ER_Core::WindowClass() const
	{
		return mWindowClass;
	}

	const std::wstring& ER_Core::WindowTitle() const
	{
		return mWindowTitle;
	}

	int ER_Core::ScreenWidth() const
	{
		return mScreenWidth;
	}

	int ER_Core::ScreenHeight() const
	{
		return mScreenHeight;
	}

	float ER_Core::AspectRatio() const
	{
		return static_cast<float>(mScreenWidth) / mScreenHeight;
	}

	const ServiceContainer& ER_Core::Services() const
	{
		return mServices;
	}
	void ER_Core::Run()
	{
		InitializeWindow();
		if (!mRHI->Initialize(mWindowHandle, mScreenWidth, mScreenHeight, mIsFullscreen))
			throw ER_CoreException("Could not initialize RHI or it is null!");
		Initialize(); //TODO ?

		MSG message;
		ZeroMemory(&message, sizeof(message));

		mCoreClock.Reset();

		while (message.message != WM_QUIT)
		{
			if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else
			{
				mCoreClock.UpdateGameTime(mCoreTime);
				Update(mCoreTime);
				Draw(mCoreTime);
			}
		}

		Shutdown();
	}

	void ER_Core::Exit()
	{
		PostQuitMessage(0);
	}

	void ER_Core::Initialize()
	{
		for (ER_CoreComponent* component : mCoreEngineComponents)
		{
			component->Initialize();
		}
	}

	void ER_Core::Shutdown()
	{
		DeleteObject(mRHI);
		UnregisterClass(mWindowClass.c_str(), mWindow.hInstance);
	}

	void ER_Core::Update(const ER_CoreTime& gameTime)
	{
		for (ER_CoreComponent* component : mCoreEngineComponents)
		{
			if (component->Enabled())
			{
				component->Update(gameTime);
			}
		}
	}

	void ER_Core::InitializeWindow()
	{
		ZeroMemory(&mWindow, sizeof(mWindow));

		mWindow.cbSize = sizeof(WNDCLASSEX);
		mWindow.style = CS_CLASSDC;
		mWindow.lpfnWndProc = WndProc;
		mWindow.hInstance = mInstance;
		mWindow.hIcon = (HICON)LoadImage(NULL, L"bunny.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		mWindow.hIconSm = (HICON)LoadImage(NULL, L"bunny.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

		mWindow.hCursor = LoadCursor(nullptr, IDC_ARROW);
		mWindow.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
		mWindow.lpszClassName = mWindowClass.c_str();

		RECT windowRectangle = { 0, 0, static_cast<LONG>(mScreenWidth), static_cast<LONG>(mScreenHeight) };
		AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE);

		RegisterClassEx(&mWindow);
		POINT center = CenterWindow(mScreenWidth, mScreenHeight);
		mWindowHandle = CreateWindow(mWindowClass.c_str(), mWindowTitle.c_str(), WS_OVERLAPPEDWINDOW, center.x, center.y, windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top, nullptr, nullptr, mInstance, nullptr);

		ShowWindow(mWindowHandle, mShowCommand);
		UpdateWindow(mWindowHandle);
	}

	LRESULT WINAPI ER_Core::WndProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
	{
		extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(windowHandle, message, wParam, lParam))
			return true;

		switch (message)
		{

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(windowHandle, message, wParam, lParam);
	}

	POINT ER_Core::CenterWindow(int windowWidth, int windowHeight)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		POINT center;
		center.x = (screenWidth - windowWidth) / 2;
		center.y = (screenHeight - windowHeight) / 2;

		return center;
	}
}