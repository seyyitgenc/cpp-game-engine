// GameEngineFromScratch.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	const wchar_t CLASS_NAME[] = L"Sample Window Class";
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	//creating the window
	HWND hwnd = CreateWindowEx(
		0,//optional window style
		CLASS_NAME,//window class
		L"Lean to program winfows",//window name
		WS_OVERLAPPEDWINDOW,//windows style
		//size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,//parent window
		NULL,//menu
		hInstance,//instance handle
		NULL//additional application data
	);

	if (hwnd == NULL)
		return 0;
	ShowWindow(hwnd, nCmdShow);
	MSG msg = {};
	//TODO: Main Game Loop must include this
	//event loop
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
	{
		int width = LOWORD(lParam);//macro to get the low-order word
		int height = HIWORD(lParam);//macro to get the high-order word

		//respond to the message
		//OnSize(hwnd, (UINT)wParam, width, height);
		std::cout << "RESIZED" << std::endl;
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		//all painting occurs here, between BeginPaint and End Paint
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;
	case WM_CLOSE:
		//PostQuitMessage(0) is also ending the loop you can use it on event loop
		if (MessageBox(hwnd, L"Really Quit", L"My Application", MB_OKCANCEL) == IDOK)
			PostQuitMessage(0);

		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
void OnSize(HWND hwnd, UINT flag, int width, int height) {
	//handle resizing
}