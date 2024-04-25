#pragma once

void blend2d_winapi_window_resize(int cx, int cy);
void blend2d_winapi_paint(HDC hdc);
void blend2d_winapi_init(HWND hwnd);
void blend2d_winapi_term(void);

void OnHScroll(WPARAM wparam, LPARAM lparam);
void OnVScroll(WPARAM wparam, LPARAM lparam);
void OnMouseWheel(WPARAM wparam, LPARAM lparam);