﻿/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#include "framework.h"
#include "windowsx.h"
#include "App.h"
#include "blend2d_winapi/blend2d_winapi.hpp"





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#define MAX_LOADSTRING 100





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
INT_PTR CALLBACK About(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}


	return (INT_PTR)FALSE;
}





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	//--------------------------------------------------------------------------
	case WM_CREATE:
	{
		OnCreate(hWnd, uMsg, wParam, lParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_DESTROY:
	{
		OnDestroy(hWnd, uMsg, wParam, lParam);

		PostQuitMessage(0);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_SIZE:
	{
		OnSize(hWnd, uMsg, wParam, lParam);

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_PAINT:
	{
		OnPaint(hWnd, uMsg, wParam, lParam);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_ERASEBKGND:
	{
		//return DefWindowProc(hWnd, uMsg, wParam, lParam);
		return 1;
	}
	break;

	//--------------------------------------------------------------------------
	case WM_HSCROLL: 
	{
		OnHScroll(hWnd, uMsg, wParam, lParam);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_VSCROLL:
	{
		OnVScroll(hWnd, uMsg, wParam, lParam);
	}
	break;

	//--------------------------------------------------------------------------
	case WM_MOUSEWHEEL:
	{
		OnMouseWheel(hWnd, uMsg, wParam, lParam);
	}	break;

	//--------------------------------------------------------------------------
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}
	break;

	//--------------------------------------------------------------------------
	default:
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	break;
	}


	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_APP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_APP, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);


	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}


	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_APP));

	MSG msg;

	
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	return (int)msg.wParam;
}
