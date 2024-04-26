#pragma once

void OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void OnHScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnVScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnMouseWheel(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);