/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#pragma comment(lib, "D:/blend2d/install/lib/blend2d.lib")
#if 0
#if defined(_DEBUG)
#pragma comment(lib, "vld.lib")
#endif
#endif





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#include <vector>
#include <chrono>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>

//===========================================================================
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>

//===========================================================================
#include <blend2d.h>





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class stopwatch
{
public:
	std::chrono::system_clock::time_point _start{};
	std::chrono::system_clock::time_point _stop{};
	std::chrono::microseconds _duration{};
	std::string _name;

public:
	explicit stopwatch(std::string name) :
		_name{ name }
	{
	}

	~stopwatch()
	{
	}

	void start(void)
	{
		_start = std::chrono::system_clock::now();
	}

	void stop(void)
	{
		_stop = std::chrono::system_clock::now();
	}

	void measure(void)
	{
		_duration = std::chrono::duration_cast<std::chrono::microseconds>(_stop - _start);
	}

	void print(void)
	{
		std::ostringstream _oss;


		_oss 
			<< "[" 
			<< _name 
			<< "] "
			<< "duration: " 
			<< _duration.count() 
			<< "usec" 
			<< std::endl;


		OutputDebugStringA(_oss.str().c_str());
	}
};

class scoped_time_measurer
{
private:
	stopwatch* _stopwatch;

public:
	explicit scoped_time_measurer(stopwatch* sw) :
		_stopwatch{ sw }
	{
		_stopwatch->start();
	}

	~scoped_time_measurer()
	{
		_stopwatch->stop();
		_stopwatch->measure();
		_stopwatch->print();
	}
};





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#if  0
https://www.codeproject.com/Articles/1042516/Custom-Controls-in-Win-API-Scrolling

typedef struct tagSCROLLINFO {
	UINT cbSize;
	UINT fMask;
	int  nMin;
	int  nMax;
	UINT nPage;
	int  nPos;
	int  nTrackPos;
} SCROLLINFO, * LPSCROLLINFO;



https://www.catch22.net/tuts/win32/64bit-scrollbars/

void example(HWND hwnd)
{
	UINT64 max = 0xffffffffffffffff;
	UINT64 pos = 0x1234567812345678;

	SetScrollInfo64(hwnd, SB_VERT, SIF_ALL, max, pos, 15, TRUE);

	// when reacting to a WM_VSCROLL, with SB_THUMBTRACK/SB_THUMBPOS:
	pos = GetScrollPos64(hwnd, SB_VERT, SIF_TRACKPOS, max);
}

#endif

#define WIN16_SCROLLBAR_MAX 0x7fff
#define WIN32_SCROLLBAR_MAX 0x7fffffff

// Wrapper around SetScrollInfo, performs scaling to 
// allow massive 64bit scroll ranges
BOOL SetScrollInfo64(HWND hwnd,
	int nBar,
	UINT fMask,
	UINT64 nMax64,
	UINT64 nPos64,
	int nPage,
	BOOL fRedraw
)
{
	SCROLLINFO si = { static_cast<UINT>(sizeof(si)), fMask };

	// normal scroll range requires no adjustment
	if (nMax64 <= WIN32_SCROLLBAR_MAX)
	{
		si.nMin = (int)0;
		si.nMax = (int)nMax64;
		si.nPage = (int)nPage;
		si.nPos = (int)nPos64;
	}
	// scale the scrollrange down into allowed bounds
	else
	{
		si.nMin = (int)0;
		si.nMax = (int)WIN16_SCROLLBAR_MAX;
		si.nPage = (int)nPage;
		si.nPos = (int)(nPos64 / (nMax64 / WIN16_SCROLLBAR_MAX));
	}

	return SetScrollInfo(hwnd, nBar, &si, fRedraw);
}

UINT64 GetScrollPos64(HWND hwnd,
	int nBar,
	UINT fMask,
	UINT64 nMax64
)
{
	SCROLLINFO si = { static_cast<UINT>(sizeof(si)), fMask | SIF_PAGE };
	UINT64 nPos32;


	if (!GetScrollInfo(hwnd, nBar, &si))
	{
		return 0;
	}


	nPos32 = (fMask & SIF_TRACKPOS) ? si.nTrackPos : si.nPos;

	// special-case: scroll position at the very end
	if (nPos32 == WIN16_SCROLLBAR_MAX - si.nPage + 1)
	{
		return nMax64 - si.nPage + 1;
	}// normal scroll range requires no adjustment
	else if (nMax64 <= WIN32_SCROLLBAR_MAX)
	{
		return nPos32;
	}
	// adjust the scroll position to be relative to maximum value
	else
	{
		return nPos32 * (nMax64 / WIN16_SCROLLBAR_MAX);
	}
}





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class bitmap32
{
private:
	std::uint8_t* _data{ nullptr };
	std::size_t _data_size{ 0 };
	BITMAPINFO _bmi{};

private:
	std::size_t       _cy{ 0 };
	std::size_t       _cx{ 0 };
	std::size_t       _aligned_scanline_cx_bytes{ 0 };
	const std::size_t _color_bits = 32;

public:
	~bitmap32()
	{
		destroy();
	}

private:
	void create(void)
	{
		memset(&_bmi, 0, sizeof(_bmi));
		_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		_bmi.bmiHeader.biWidth = static_cast<LONG>(_cx);
		_bmi.bmiHeader.biHeight = -static_cast<LONG>(_cy);
		_bmi.bmiHeader.biPlanes = 1;
		_bmi.bmiHeader.biCompression = BI_RGB;
		_bmi.bmiHeader.biBitCount = static_cast<WORD>(_color_bits);


		auto raw_scanline_width_in_bits{ _cx * _color_bits };

		auto aligned_bits{ 32 - 1 };
		auto aligned_scanline_width_in_bits{ (raw_scanline_width_in_bits + aligned_bits) & ~aligned_bits };
		auto aligned_scanline_width_in_bytes{ aligned_scanline_width_in_bits / 8 };


		_aligned_scanline_cx_bytes = aligned_scanline_width_in_bytes;


		_data_size = _aligned_scanline_cx_bytes * _cy;

		_data = new (std::nothrow) std::uint8_t[_data_size];
	}

	void destroy(void)
	{
		if (nullptr != _data)
		{
			delete[] _data;
		}
		_data = nullptr;
	}

public:
	bool is_empty(void)
	{
		if (nullptr == _data) return true;

		return false;
	}

	void set_size(std::size_t cx, std::size_t cy)
	{
		if (!is_empty())
		{
			destroy();
		}


		_cx = cx;
		_cy = cy;


		create();
	}

	std::size_t get_scanline_bytes(void)
	{
		return _aligned_scanline_cx_bytes;
	}

	std::uint8_t* get_data(void)
	{
		return _data;
	}

	BITMAPINFO* get_bitmap_info(void)
	{
		return &_bmi;
	}

	std::size_t get_cx(void)
	{
		return _cx;
	}

	std::size_t get_cy(void)
	{
		return _cy;
	}
};





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class canvas
{
private:
	bitmap32 _bitmap{};

private:
	std::size_t _cy{ 0 };
	std::size_t _cx{ 0 };

private:
	BLImage _image;
	BLContext _context; 

public:
	void set_size(std::size_t cx, std::size_t cy)
	{
		bool reset = false;


		_cx = cx;
		_cy = cy;


		_bitmap.set_size(cx, cy);

		if (cx == 0)
		{
			reset = true;
		}

		if (cx == 0)
		{
			reset = true;
		}

		if (_bitmap.is_empty())
		{
			reset = true;
		}


		BLResult result;


		if (reset)
		{
			_image.reset();
		}
		else
		{
			result = _image.createFromData(
				static_cast<int>(_cx), static_cast<int>(_cy),
				BL_FORMAT_PRGB32,
				_bitmap.get_data(),
				_bitmap.get_scanline_bytes()
			);
			if (result != BL_SUCCESS)
			{
				printf("_image.createFromData() (%u)\n", result);
			}
		}

		// 그릴때마다 생성이 필요
		_context = BLContext(_image);
	}

	void reset(void)
	{
		/*
		BLResult result;

			result = _image.createFromData(
				static_cast<int>(_cx), static_cast<int>(_cy),
				BL_FORMAT_PRGB32,
				_bitmap.get_data(),
				_bitmap.get_scanline_bytes()
			);
			if (result != BL_SUCCESS)
			{
				printf("_image.createFromData() (%u)\n", result);
			}
			*/
		BLContextCreateInfo createInfo{};
		createInfo.threadCount = 8;
			_context = BLContext(_image, createInfo);
	}

	BLContext* get_context(void)
	{
		return &_context;
	}

	void paint(HDC hdc)
	{
		if (_bitmap.is_empty())
		{
			return;
		}

		StretchDIBits(hdc,
			static_cast<int>(0), static_cast<int>(0), static_cast<int>(_cx), static_cast<int>(_cy),
			static_cast<int>(0), static_cast<int>(0), static_cast<int>(_cx), static_cast<int>(_cy),
			_bitmap.get_data(),
			_bitmap.get_bitmap_info(),
			DIB_RGB_COLORS, SRCCOPY);
	}
};




/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class blend2d_winapi
{
public:
	double _document_x{ 0 };
	double _document_y{ 0 };
	double _document_cx { 1920*2 };
	double _document_cy { 1080*2 };

	double _scale{ 1.0 };

	std::int64_t _view_x {0};
	std::int64_t _view_y {0};
	std::int64_t _view_cx{0};
	std::int64_t _view_cy{0};

	std::int64_t _window_cx{0};
	std::int64_t _window_cy{0};

public:
	canvas _canvas;
	BLFontFace _font_face;
	BLFont _font;
	HWND _hwnd;

public:
	void create(HWND hwnd)
	{
		//-------------------------------------------------------------------
		_hwnd = hwnd;


		//-------------------------------------------------------------------
		BLResult result;


		result = _font_face.createFromFile("C:/Windows/Fonts/malgun.ttf");
		if (result != BL_SUCCESS)
		{
			printf("_font_face.createFromFile() (%u)\n", result);
		}

		_font.createFromFace(_font_face, 50.0f);
	}

	void destory()
	{

	}

	void window_resize(int cx, int cy)
	{
		_window_cx = cx;
		_window_cy = cy;

		_canvas.set_size(cx, cy);

		//recalc_view();
	}

	double get_scale(void)
	{
		return _scale;
	}

	void repaint(void)
	{
		//_canvas.get_context()->reset();
		//InvalidateRect(_hwnd, nullptr, TRUE);
		//UpdateWindow(_hwnd);

		/*
		HDC hdc = GetDC(_hwnd);
		paint(hdc);
		ReleaseDC(_hwnd, hdc);
		*/

		InvalidateRect(_hwnd, nullptr, TRUE);
	}

	void set_scale(double s)
	{
		_scale = s;

		recalc_view();

		repaint();
	}

	void recalc_view(void)
	{
		_view_x = static_cast<std::int64_t>(_document_x * _scale);
		_view_y = static_cast<std::int64_t>(_document_y * _scale);

		_view_cx = static_cast<std::int64_t>(_document_cx * _scale);
		_view_cy = static_cast<std::int64_t>(_document_cy * _scale);
	}

public:
	void paint(HDC hdc)
	{
		BLContext* ctx;



		if (0 == _window_cx)
		{
			return;
		}
		if (0 == _window_cy)
		{
			return;
		}


		ctx = _canvas.get_context();

		_canvas.reset();
		draw(ctx);


		{
			stopwatch sw("gdi");
			scoped_time_measurer stm(&sw);

			_canvas.paint(hdc);
		}
	}

	void draw(BLContext* ctx)
	{
		stopwatch sw("blend2d");
		scoped_time_measurer stm(&sw);

		BLContextCookie context_cookie;


		ctx->save(context_cookie);


		ctx->scale(_scale);
		ctx->translate(static_cast<double>(_view_x), static_cast<double>(_view_y));
		draw_document(ctx);


		ctx->restore(context_cookie);

		ctx->end();
	}

	void draw_document(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);


		ctx->clearAll();


		draw_ex5(ctx);
		draw_ex7(ctx);
		draw_t1(ctx);


		ctx->restore(context_cookie);
	}

	void draw_ex5(BLContext* ctx)
	{
		// First shape filled with a radial gradient.
		// By default, SRC_OVER composition is used.
		ctx->setCompOp(BL_COMP_OP_SRC_OVER);

		BLGradient radial(
			BLRadialGradientValues(180, 180, 180, 180, 180));
		radial.addStop(0.0, BLRgba32(0xFFFFFFFF));
		radial.addStop(1.0, BLRgba32(0xFFFF6F3F));
		ctx->fillCircle(180, 180, 160, radial);

		// Second shape filled with a linear gradient.
		BLGradient linear(
			BLLinearGradientValues(195, 195, 470, 470));
		linear.addStop(0.0, BLRgba32(0xFFFFFFFF));
		linear.addStop(1.0, BLRgba32(0xFF3F9FFF));

		// Use 'setCompOp()' to change a composition operator.
		ctx->setCompOp(BL_COMP_OP_DIFFERENCE);
		ctx->fillRoundRect(
			BLRoundRect(195, 195, 270, 270, 25), linear);

		ctx->setCompOp(BL_COMP_OP_SRC_OVER);
	}

	void draw_ex7(BLContext* ctx)
	{
		const char regularText[] = "Hello Blend2D!";
		const char rotatedText[] = u8"Rotated Text 한글";

		BLContextCookie cookie1;
		BLContextCookie cookie2;

		ctx->save(cookie1);
		ctx->scale(0.5);
		{
			//BLFont _font;
			//_font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFFFF0000));
			ctx->fillUtf8Text(BLPoint(60, 80), _font, regularText);

			//ctx->rotate(0.785398);
			//ctx->fillUtf8Text(BLPoint(250, 80), _font, rotatedText);
		}

		ctx->save(cookie2);
		ctx->scale(0.5);
		{

			//BLFont _font;
			//_font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFFFFFFFF));
			ctx->fillUtf8Text(BLPoint(60, 80), _font, regularText);

			//ctx->rotate(0.785398);
			//ctx->fillUtf8Text(BLPoint(250, 80), _font, rotatedText);
		}

		ctx->restore(cookie2);
		ctx->restore(cookie1);


		ctx->save(cookie1);

		ctx->translate(100, 100);

		{
			//BLFont _font;
			//_font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFF0000FF));
			ctx->fillUtf8Text(BLPoint(60, 80), _font, regularText);

			ctx->rotate(0.785398);
			ctx->fillUtf8Text(BLPoint(250, 80), _font, rotatedText);
		}
		ctx->restore(cookie1);
	}

	void draw_t1(BLContext* ctx)
	{
		//ctx->fillAll(BLRgba32(0xFF000000u));
		double _angle = 45;

		BLPath p;

		int count = 1000;
		double PI = 3.14159265359;

		double cx = _window_cx / 2.0f;
		double cy = _window_cy / 2.0f;
		double maxDist = 1000.0;
		double baseAngle = _angle / 180.0 * PI;

		for (int i = 0; i < count; i++) {
			double t = double(i) * 1.01 / 1000;
			double d = t * 1000.0 * 0.4 + 10;
			double a = baseAngle + t * PI * 2 * 20;
			double x = cx + std::cos(a) * d;
			double y = cy + std::sin(a) * d;
			double r = std::min(t * 8 + 0.5, 10.0);
			p.addCircle(BLCircle(x, y, r));
		}

		ctx->fillPath(p, BLRgba32(0xFF00FFFFu));
	}
};





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
static blend2d_winapi* _blend2d_winapi;





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
void blend2d_winapi_init(HWND hwnd)
{
	//-----------------------------------------------------------------------
	_blend2d_winapi = new blend2d_winapi();

	_blend2d_winapi->create(hwnd);
}

void blend2d_winapi_term(void)
{
	//-----------------------------------------------------------------------
	_blend2d_winapi->destory();

	delete _blend2d_winapi;
}

void blend2d_winapi_window_resize(int cx, int cy)
{
	_blend2d_winapi->window_resize(cx, cy);
}

void blend2d_winapi_paint(HDC hdc)
{
	_blend2d_winapi->paint(hdc);
}

void OnHScroll(WPARAM wparam, LPARAM lparam)
{
}

void OnVScroll(WPARAM wparam, LPARAM lparam)
{

}

void OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	UINT nFlags{ (UINT)LOWORD(wParam) };
	short zDelta{ (short)HIWORD(wParam) };
	POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	bool scale = false;

	switch (nFlags)
	{
	case MK_CONTROL:
		scale = true;
		break;
	case MK_LBUTTON:
	case MK_RBUTTON:
		break;
	case MK_MBUTTON:
		break;
	case MK_SHIFT:
		break;
	case MK_XBUTTON1:
	case MK_XBUTTON2:
		break;
	}

	if (scale)
	{
		double viewscale_delta;
		double viewscale_max;
		double viewscale_min;
		double viewscale;
		double previous_viewscale;


		viewscale = _blend2d_winapi->get_scale();
		viewscale_max = 5.0f;
		viewscale_min = 0.1f;
		viewscale_delta = 0.1f;

		previous_viewscale = viewscale;



		if (zDelta > 0)
		{
			viewscale = viewscale + viewscale_delta;
		}
		else
		{
			viewscale = viewscale - viewscale_delta;
		}

		if (viewscale > viewscale_max)
		{
			viewscale = viewscale_max;
		}
		if (viewscale < viewscale_min)
		{
			viewscale = viewscale_min;
		}

		_blend2d_winapi->set_scale(viewscale);
	}
	else
	{
		if (zDelta > 0)
		{
			OnVScroll(MAKEWORD(SB_LINEUP, 0), 0);
		}
		else
		{
			OnVScroll(MAKEWORD(SB_LINEDOWN, 0), 0);
		}
	}

}


