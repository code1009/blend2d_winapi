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
	UINT64 nPage,
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

		if (_data_size)
		{
			_data = new (std::nothrow) std::uint8_t[_data_size];
		}
		else
		{
			_data = nullptr;
		}
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
		if (nullptr == _data)
		{
			return true;
		}

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
	BLImage _image{};
	BLContext _context{};
	BLContext* _context_ptr{ nullptr };

private:
	std::size_t _cy{ 0 };
	std::size_t _cx{ 0 };

public:
	void set_size(std::size_t cx, std::size_t cy)
	{
		bool reset = false;


		_cx = cx;
		_cy = cy;


		_bitmap.set_size(cx, cy);
		
		if (_bitmap.get_data())
		{
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
		}
	}

	BLContext* begin(void)
	{
		if (_bitmap.get_data())
		{
			BLContextCreateInfo createInfo{};


			createInfo.threadCount = 1;
			
			//_context.begin(_image, createInfo);
			_context = BLContext(_image, createInfo);

			_context_ptr = &_context;
		}
		else
		{
			_context_ptr = nullptr;
		}

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

		return _context_ptr;
	}

	void end(void)
	{
		if (_context_ptr)
		{
			_context_ptr->end();
		}

		_context_ptr = nullptr;
	}

	void paint(HDC hdc)
	{
		if (_bitmap.get_data())
		{
			StretchDIBits(hdc,
				static_cast<int>(0), static_cast<int>(0), static_cast<int>(_cx), static_cast<int>(_cy),
				static_cast<int>(0), static_cast<int>(0), static_cast<int>(_cx), static_cast<int>(_cy),
				_bitmap.get_data(),
				_bitmap.get_bitmap_info(),
				DIB_RGB_COLORS, SRCCOPY);
		}
	}
};

std::int64_t on_scroll(
	std::uint32_t scroll_code, std::int64_t pos,
	std::uint64_t _view_y_scroll_page,
	std::uint64_t _view_y_scroll_line,
	std::uint64_t _view_y_scroll_min,
	std::uint64_t _view_y_scroll_max,
	std::uint64_t _view_y
	)
{
	std::int64_t view_scroll_min;
	std::int64_t view_scroll_max;
	std::int64_t view_scroll_top;
	std::int64_t view_scroll_bottom;
	std::int64_t view_scroll_pos;
	std::int64_t view_scroll_pos_current;
	std::int64_t view_scroll_page;
	std::int64_t view_scroll_line;


	view_scroll_page = _view_y_scroll_page;
	view_scroll_line = _view_y_scroll_line;


	view_scroll_min = _view_y_scroll_min;
	view_scroll_max = _view_y_scroll_max;


	view_scroll_top = view_scroll_min;
	if (view_scroll_page < view_scroll_max)
	{
		view_scroll_bottom = view_scroll_max - view_scroll_page;
	}
	else
	{
		view_scroll_bottom = view_scroll_min;
	}


	view_scroll_pos = _view_y;
	view_scroll_pos_current = _view_y;


	switch (scroll_code)
	{
	case SB_TOP:
		view_scroll_pos_current = view_scroll_top;
		break;

	case SB_BOTTOM:
		view_scroll_pos_current = view_scroll_bottom;
		break;

	case SB_LINEUP:
		if (view_scroll_top < (view_scroll_pos - view_scroll_line))
		{
			view_scroll_pos_current = view_scroll_pos - view_scroll_line;
		}
		else
		{
			view_scroll_pos_current = view_scroll_top;
		}
		break;

	case SB_LINEDOWN:
		if ((view_scroll_pos + view_scroll_line) < view_scroll_bottom)
		{
			view_scroll_pos_current = view_scroll_pos + view_scroll_line;
		}
		else
		{
			view_scroll_pos_current = view_scroll_bottom;
		}
		break;

	case SB_PAGEUP:
		if (view_scroll_top < (view_scroll_pos - view_scroll_page))
		{
			view_scroll_pos_current = view_scroll_pos - view_scroll_page;
		}
		else
		{
			view_scroll_pos_current = view_scroll_top;
		}
		break;

	case SB_PAGEDOWN:
		if ((view_scroll_pos + view_scroll_page) < view_scroll_bottom)
		{
			view_scroll_pos_current = view_scroll_pos + view_scroll_page;
		}
		else
		{
			view_scroll_pos_current = view_scroll_bottom;
		}
		break;

	case SB_THUMBTRACK:
		view_scroll_pos_current = pos;
		break;

	case SB_THUMBPOSITION:
		view_scroll_pos_current = pos;
		break;

	case SB_ENDSCROLL:
		break;
	}

	if (view_scroll_pos_current < view_scroll_top)
	{
		view_scroll_pos_current = view_scroll_top;
	}

	if (view_scroll_bottom < view_scroll_pos_current)
	{
		view_scroll_pos_current = view_scroll_bottom;
	}


	return view_scroll_pos_current;
}


/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class window
{
private:
	double _contents_x{ 0 };
	double _contents_y{ 0 };
	double _contents_cx { 0 };
	double _contents_cy { 0 };

	double _scale{ 1.0 };

	std::int64_t _view_x {0};
	std::int64_t _view_y {0};
	std::int64_t _view_cx{0};
	std::int64_t _view_cy{0};

	std::int64_t _view_x_scroll_min { 0 };
	std::int64_t _view_x_scroll_max { 0 };
	std::int64_t _view_x_scroll_page{ 0 };
	std::int64_t _view_x_scroll_line{ 0 };

	std::int64_t _view_y_scroll_min { 0 };
	std::int64_t _view_y_scroll_max { 0 };
	std::int64_t _view_y_scroll_page{ 0 };
	std::int64_t _view_y_scroll_line{ 0 };

	std::int64_t _window_cx{0};
	std::int64_t _window_cy{0};
	
	std::int64_t _paint_time_usec{ 0 };

private:
	canvas _canvas;
	BLFontFace _font_face;
	BLFontFace _font_face_status;
	BLFont _font;
	BLFont _underlay_font;
	BLFont _overlay_font;
	HWND _hwnd;
	bool _scrollbar_enabled{ false };

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

		result = _font_face_status.createFromFile("C:/Users/USER/AppData/Local/Microsoft/Windows/Fonts/NanumGothicCoding-Bold.ttf");
		if (result != BL_SUCCESS)
		{
			printf("_font_face.createFromFile() (%u)\n", result);
		}
		

		_font.createFromFace(_font_face, 50.0f);
		_underlay_font.createFromFace(_font_face_status, 12.0f);
		_overlay_font.createFromFace(_font_face_status, 16.0f);
	}

	void destory()
	{

	}

public:
	void set_window_size(int cx, int cy)
	{
		_window_cx = cx;
		_window_cy = cy;

		
		_canvas.set_size(cx, cy);
		
		
		update_view_size();
		update_view_offset();
		update_view_scroll();
		update_scrollbar();


		//repaint(); 호출 금지
		//윈도우 최초 실행을 제외 하고
		//WM_SIZE이후 WM_PAINT호출되기떄문에 다시 그릴 필요 없음
	}

	double get_scale(void)
	{
		return _scale;
	}
	
	void set_scale(double s)
	{
		if (_scale > 0.0)
		{
			_scale = s;

			update_view_size();
			update_view_offset();
			update_view_scroll();
			update_scrollbar();

			repaint();
		}
	}

	void set_contents_size(double cx, double cy)
	{
		_contents_x  = 0;
		_contents_y  = 0;
		_contents_cx = cx;
		_contents_cy = cy;

		update_view_size();
		update_view_offset();
		update_view_scroll();
		update_scrollbar();

		repaint();
	}

	void enable_scrollbar(bool enable)
	{
		_scrollbar_enabled = enable;

		update_view_scroll();
		update_scrollbar();
	}

	void fit_contents_to_window(bool vert=false)
	{
		if (vert)
		{
			if (_window_cy)
			{
				set_scale(_window_cy / _contents_cy);
			}
		}
		else
		{
			if (_window_cx)
			{
				set_scale(_window_cx / _contents_cx);
			}
		}
	}

	void on_vscroll(std::uint32_t scroll_code)
	{
		std::int64_t pos;


		pos = GetScrollPos64(_hwnd, SB_VERT, SIF_TRACKPOS, _view_y_scroll_max);


		std::int64_t view_scroll_pos_current;
		std::int64_t view_scroll_pos;


		view_scroll_pos = _view_y;
		view_scroll_pos_current = on_scroll(scroll_code, pos,
			_view_y_scroll_page,
			_view_y_scroll_line,
			_view_y_scroll_min,
			_view_y_scroll_max,
			_view_y
			);


		if (view_scroll_pos != view_scroll_pos_current)
		{
			_view_y = view_scroll_pos_current;
			_contents_y = view_scroll_pos_current / _scale;

			update_scrollbar();

			repaint();
		}
	}

	void on_hscroll(std::uint32_t scroll_code)
	{
		std::int64_t pos;


		pos = GetScrollPos64(_hwnd, SB_HORZ, SIF_TRACKPOS, _view_x_scroll_max);


		std::int64_t view_scroll_pos_current;
		std::int64_t view_scroll_pos;


		view_scroll_pos = _view_x;
		view_scroll_pos_current = on_scroll(scroll_code, pos,
			_view_x_scroll_page,
			_view_x_scroll_line,
			_view_x_scroll_min,
			_view_x_scroll_max,
			_view_x
		);


		if (view_scroll_pos != view_scroll_pos_current)
		{
			_view_x = view_scroll_pos_current;
			_contents_x = view_scroll_pos_current / _scale;

			update_scrollbar();

			repaint();
		}
	}

private:
	void update_scrollbar(void)
	{
		SetScrollInfo64(_hwnd, SB_HORZ, SIF_ALL, _view_x_scroll_max, _view_x, _view_x_scroll_page, TRUE);
		SetScrollInfo64(_hwnd, SB_VERT, SIF_ALL, _view_y_scroll_max, _view_y, _view_y_scroll_page, TRUE);
	}
	
	void update_view_scroll(void)
	{
		if (_scrollbar_enabled)
		{
			if (_window_cx < _view_cx)
			{
				_view_x_scroll_min = 0;
				_view_x_scroll_max = _view_cx;
				_view_x_scroll_page = _window_cx;
				_view_x_scroll_line = 10;
			}
			else
			{
				_view_x_scroll_min = 0;
				_view_x_scroll_max = 0;
				_view_x_scroll_page = 0;
				_view_x_scroll_line = 0;
			}


			if (_window_cy < _view_cy)
			{
				_view_y_scroll_min = 0;
				_view_y_scroll_max = _view_cy;
				_view_y_scroll_page = _window_cy;
				_view_y_scroll_line = 10;
			}
			else
			{
				_view_y_scroll_min = 0;
				_view_y_scroll_max = 0;
				_view_y_scroll_page = 0;
				_view_y_scroll_line = 0;
			}
		}
		else
		{
			_view_x_scroll_min = 0;
			_view_x_scroll_max = 0;
			_view_x_scroll_page = 0;
			_view_x_scroll_line = 0;

			_view_y_scroll_min = 0;
			_view_y_scroll_max = 0;
			_view_y_scroll_page = 0;
			_view_y_scroll_line = 0;
		}
	}

	void update_view_size(void)
	{
		_view_cx = static_cast<std::int64_t>(_contents_cx * _scale);
		_view_cy = static_cast<std::int64_t>(_contents_cy * _scale);
	}

	void update_view_offset(void)
	{
		_view_x = static_cast<std::int64_t>(_contents_x * _scale);
		if (_view_cx < _window_cx)
		{
			_view_x = 0;
		}
		else
		{
			if (_view_cx < (_view_x + _window_cx))
			{
				_view_x = _view_cx - _window_cx;
			}
		}


		_view_y = static_cast<std::int64_t>(_contents_y * _scale);
		if (_view_cy < _window_cy)
		{
			_view_y = 0;
		}
		else
		{
			if (_view_cy < (_view_y + _window_cy))
			{
				_view_y = _view_cy - _window_cy;
			}
		}

		_contents_x = _view_x / _scale;
		_contents_y = _view_y / _scale;
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

	//--------------------------------------------------------------------------
public:
	void paint(HDC hdc)
	{
		stopwatch sw("paint");
		{
			scoped_time_measurer stm(&sw);


			BLContext* ctx;


			ctx = _canvas.begin();
			if (!ctx)
			{
				return;
			}
			draw(ctx);
			_canvas.end();


			{
				stopwatch sw("gdi");
				scoped_time_measurer stm(&sw);

				_canvas.paint(hdc);
			}
		}
		_paint_time_usec = sw._duration.count();
	}

	void draw(BLContext* ctx)
	{
		stopwatch sw("blend2d");
		scoped_time_measurer stm(&sw);


		ctx->clearAll();

		draw_underlay(ctx);
		draw_contents(ctx);
		draw_overlay(ctx);
	}

	void draw_underlay(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);


		draw_window_grid(ctx);


		ctx->restore(context_cookie);
	}

	void draw_overlay(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);

		
		draw_window_info(ctx);


		ctx->restore(context_cookie);
	}

	void draw_window_info(BLContext* ctx)
	{
		std::int64_t x;
		std::int64_t y;
		std::int64_t line;

		std::int64_t text_offset_x;
		std::int64_t text_offset_y;

		char label[128];


		line = 8;

		text_offset_x = 10;
		text_offset_y = 25;

		x = _window_cx - 360;
		y = _window_cy - (line * text_offset_y);


		ctx->setFillStyle(BLRgba32(0xC00000FF));

		/*
		ctx->fillBox(
			static_cast<double>(x), static_cast<double>(y),
			static_cast<double>(_window_cx), static_cast<double>(_window_cy)
		);
		*/

		ctx->fillRoundRect(
			static_cast<double>(x), static_cast<double>(y),
			static_cast<double>(360 - 10), static_cast<double>(line * text_offset_y - 10),
			5
			);


		ctx->setFillStyle(BLRgba32(0xFFFFFFFF));


		sprintf_s(label, "render   = %lld usec", _paint_time_usec);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;


		sprintf_s(label, "window   = (%lld, %lld)", _window_cx, _window_cy);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

		sprintf_s(label, "contents = (%.1f, %.1f)-(%.1f, %.1f)", _contents_x, _contents_y, _contents_cx, _contents_cy);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

		sprintf_s(label, "scale    = %.1f", _scale);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

		sprintf_s(label, "view     = (%lld, %lld)-(%lld, %lld)", _view_x, _view_y, _view_cx, _view_cy);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

		sprintf_s(label, "scroll x = %lld, %lld, (%lld~%lld)", 
			_view_x_scroll_line,
			_view_x_scroll_page,
			_view_x_scroll_min, _view_x_scroll_max
		);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

		sprintf_s(label, "scroll y = %lld, %lld, (%lld~%lld)",
			_view_y_scroll_line, 
			_view_y_scroll_page,
			_view_y_scroll_min, _view_y_scroll_max
		);
		ctx->fillUtf8Text(BLPoint(static_cast<double>(x + text_offset_x), y + static_cast<double>(text_offset_y)), _overlay_font, label);
		y += text_offset_y;

	}

	void draw_window_grid(BLContext* ctx)
	{
		if (1.0 < _scale)
		{
			return;
		}


		std::int64_t x;
		std::int64_t y;

		std::int64_t text_offset_x;
		std::int64_t text_offset_y;

		double contents_x;
		double contents_y;

		char label[128];


		text_offset_x = 5;
		text_offset_y = 15;


		ctx->setStrokeWidth(1);
		ctx->setStrokeStyle(BLRgba32(0x20FFFFFF));
		ctx->setFillStyle(BLRgba32(0xFFFFFFFF));

		for (y = 0; y < _window_cy; y += 100)
		{
			for (x = 0; x < _window_cx; x += 100)
			{
				ctx->strokeLine(BLPoint(static_cast<double>(x), static_cast<double>(0)), BLPoint(static_cast<double>(x), static_cast<double>(_window_cy)));
				ctx->strokeLine(BLPoint(static_cast<double>(0), static_cast<double>(y)), BLPoint(static_cast<double>(_window_cx), static_cast<double>(y)));


				contents_x = _contents_x + x / _scale;
				contents_y = _contents_y + y / _scale;

				sprintf_s(label, "(%.1f, %.1f)", contents_x, contents_y);


				ctx->fillUtf8Text(
					BLPoint(static_cast<double>(x + text_offset_x), static_cast<double>(y + text_offset_y)),
					_underlay_font, label);
			}
		}
	}

	void draw_contents_grid(BLContext* ctx)
	{
		if (_scale < 1.0)
		{
			return;
		}


		double x;
		double y;

		char label[128];


		ctx->setStrokeWidth(1);
		//ctx->setStrokeStartCap(BL_STROKE_CAP_ROUND);
		//ctx->setStrokeEndCap(BL_STROKE_CAP_BUTT);
		ctx->setStrokeStyle(BLRgba32(0x20FFFFFF));
		ctx->setFillStyle(BLRgba32(0xFFFFFFFF));

		for (y = 0.0; y < _contents_cy; y += 100.0)
		{
			for (x = 0.0; x < _contents_cx; x += 100.0)
			{
				ctx->strokeLine(BLPoint(x, 0), BLPoint(x, _contents_cy));
				ctx->strokeLine(BLPoint(0, y), BLPoint(_contents_cx, y));


				sprintf_s(label, "(%.1f, %.1f)", x, y);


				ctx->fillUtf8Text(BLPoint(x + 5, y + 15), _underlay_font, label);
			}
		}
	}

	void draw_contents(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);


		ctx->scale(_scale);
		ctx->translate(-_contents_x, -_contents_y);


		draw_contents_background(ctx);
		draw_contents_foreground(ctx);


		ctx->restore(context_cookie);
	}

	void draw_contents_background(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);


		ctx->setFillStyle(BLRgba32(0x40808080));
		ctx->fillBox(0, 0, _contents_cx, _contents_cy);

		draw_contents_grid(ctx);


		ctx->restore(context_cookie);
	}

	void draw_contents_foreground(BLContext* ctx)
	{
		BLContextCookie context_cookie;


		ctx->save(context_cookie);


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

		double cx = _contents_cx / 2.0f;
		double cy = _contents_cy / 2.0f;
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
static window* _blend2d_window;





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
void OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//-----------------------------------------------------------------------
	_blend2d_window = new window();

	
	_blend2d_window->create(hWnd);


	//-----------------------------------------------------------------------
	RECT rect;


	GetClientRect(hWnd, &rect);


	// 최초 윈도우 생성시 WM_PAINT가 먼저 수행 되고
	// WM_SIZE가 나중에 수행 된다.
	// 최초 실행시 윈도우 크기를 먼저 지정하지 않으면,
	// 크기가 지정되지 않은 채로 그리기 동작이 먼저 수행한다.
	// 따라서, 먼저 윈도우 크기를 지정한다.
	// 그 이후에 WM_SIZE 이후에 WM_PAINT가 수행 된다.

	_blend2d_window->set_window_size(rect.right, rect.bottom);
	_blend2d_window->set_contents_size(1920, 1080);
	_blend2d_window->fit_contents_to_window(true);
	_blend2d_window->enable_scrollbar(true);
}

void OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	_blend2d_window->destory();

	_blend2d_window = nullptr;
}

void OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT nType = (UINT)wParam;
	SIZE size = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }; // lParam specifies the new of the client area


	_blend2d_window->set_window_size(size.cx, size.cy);
//	_blend2d_window->fit_contents_to_window(true);
}

void OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	
	
	hdc = BeginPaint(hWnd, &ps);


	_blend2d_window->paint(hdc);


	EndPaint(hWnd, &ps);
}

void OnHScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT nSBCode    = (int)LOWORD(wParam);
	UINT nPos       = (short)(HIWORD(wParam));
	HWND pScrollBar = (HWND)lParam;


	_blend2d_window->on_hscroll(nSBCode);
}

void OnVScroll(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT nSBCode    = (int)LOWORD(wParam);
	UINT nPos       = (short)HIWORD(wParam);
	HWND pScrollBar = (HWND)lParam ;


	_blend2d_window->on_vscroll(nSBCode);
}

void OnMouseWheel(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT  nFlags = (UINT)LOWORD(wParam);
	short zDelta = (short)HIWORD(wParam);
	POINT pt     = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };


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


		viewscale_max = 10.0f;
		viewscale_min = 0.1f;
		viewscale_delta = 0.1f;


		double viewscale;
		int viewscale_x10;


		viewscale = _blend2d_window->get_scale();
		viewscale_x10 = static_cast<int>((viewscale + 0.05) * 10);
		viewscale = viewscale_x10 / 10.0;


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


		_blend2d_window->set_scale(viewscale);
	}
	else
	{
		if (zDelta > 0)
		{
			OnVScroll(hWnd, 0, MAKEWORD(SB_LINEUP, 0), 0);
		}
		else
		{
			OnVScroll(hWnd, 0, MAKEWORD(SB_LINEDOWN, 0), 0);
		}
	}
}


