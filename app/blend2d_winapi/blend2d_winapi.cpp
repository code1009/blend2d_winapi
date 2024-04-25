/////////////////////////////////////////////////////////////////////////////
//===========================================================================
#pragma comment(lib, "gdiplus.lib")
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
#include <gdiplus.h>

//===========================================================================
#include <blend2d.h>





/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class stopwatch
{
public:
	std::chrono::system_clock::time_point _start;
	std::chrono::system_clock::time_point _stop;
	std::chrono::microseconds _duration;
	std::string _name;

public:
	explicit stopwatch(std::string name) :
		_name { name }
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


		_oss << _name << " duration: " << _duration.count() << "usec" << std::endl;


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
class blend2d_winapi
{
public:
	void paint(HWND hwnd, HDC hdc)
	{
		{
			stopwatch sw("paint blend2d");
			scoped_time_measurer stm(&sw);
		
			paint_blend2d();
		}

		{
			stopwatch sw("paint gdi");
			scoped_time_measurer stm(&sw);

			StretchDIBits(hdc,
				static_cast<int>(0), static_cast<int>(0), static_cast<int>(_bitmap_cx), static_cast<int>(_bitmap_cy),
				static_cast<int>(0), static_cast<int>(0), static_cast<int>(_bitmap_cx), static_cast<int>(_bitmap_cy),
				_bitmap_data,
				&_bmi,
				DIB_RGB_COLORS, SRCCOPY); // 389usec
		}
#if 0
//		Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromHWND(hwnd, FALSE);
		Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromHDC(hdc);

#if 0
		pGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		pGraphics->SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
		pGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
		pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
		pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeDefault);
#endif

		pGraphics->DrawImage(_bitmap, 0, 0); // 10~20msec

		delete pGraphics;
#endif
	}

public:
	void create()
	{
		create_bitmap();
		create_blend2d();
	}

	void destory()
	{
		destroy_blend2d();
		destroy_bitmap();
	}

public:
	Gdiplus::Bitmap* _bitmap;
	std::uint8_t* _bitmap_data;
	std::size_t _bitmap_data_size;
	std::size_t _bitmap_cy = 1480;
	std::size_t _bitmap_cx = 480;
	BITMAPINFO _bmi;

	void create_bitmap()
	{
		_bitmap_data_size = _bitmap_cx * _bitmap_cy * 4;



		memset(&_bmi, 0, sizeof(_bmi));
		_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		_bmi.bmiHeader.biWidth = static_cast<LONG>(_bitmap_cx);
		_bmi.bmiHeader.biHeight = -static_cast<LONG>(_bitmap_cy);
		_bmi.bmiHeader.biPlanes = 1;
		_bmi.bmiHeader.biCompression = BI_RGB;
		_bmi.bmiHeader.biBitCount = 32;

		_bitmap_data = new std::uint8_t[_bitmap_data_size];

		_bitmap = new Gdiplus::Bitmap(&_bmi, _bitmap_data);

		memset(_bitmap_data, 0xA0, _bitmap_data_size);
	}

	void destroy_bitmap()
	{
		delete _bitmap;
		delete[] _bitmap_data;
	}

public:
	BLImage _image;
	BLContext _context;
	BLFontFace _font_face;

	void create_blend2d()
	{
		BLResult result = _font_face.createFromFile("C:/Windows/Fonts/malgun.ttf");
		if (result != BL_SUCCESS) 
		{
			printf("Failed to load a font (err=%u)\n", result);
		}

		/*
		_image = BLImage(
			static_cast<int>(_bitmap_cx), static_cast<int>(_bitmap_cy), 
			BL_FORMAT_PRGB32
		);
		*/
		_image.createFromData(
			static_cast<int>(_bitmap_cx), static_cast<int>(_bitmap_cy),
			BL_FORMAT_PRGB32, 
			_bitmap_data, 
			_bitmap_cx*4
		);


		_context = BLContext(_image);
	}

	void destroy_blend2d()
	{
		//delete _context;
		//delete _image;
	}

	void paint_blend2d()
	{
		_context.clearAll();
		paint_ex(&_context);
		_context.end();
	}

	void paint_ex(BLContext* ctx)
	{
		paint_ex5(ctx);
		paint_ex7(ctx);
		paint_t1(ctx);
	}

	void paint_ex5(BLContext* ctx)
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

	void paint_ex7(BLContext* ctx)
	{
		const char regularText[] = "Hello Blend2D!";
		const char rotatedText[] = u8"Rotated Text ÇÑ±Û";

		BLContextCookie cookie1;
		BLContextCookie cookie2;

		ctx->save(cookie1);
		ctx->scale(0.5);
		{
			BLFont font;
			font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFFFF0000));
			ctx->fillUtf8Text(BLPoint(60, 80), font, regularText);

			//ctx->rotate(0.785398);
			//ctx->fillUtf8Text(BLPoint(250, 80), font, rotatedText);
		}

		ctx->save(cookie2);
		ctx->scale(0.5);
		{

			BLFont font;
			font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFFFFFFFF));
			ctx->fillUtf8Text(BLPoint(60, 80), font, regularText);

			//ctx->rotate(0.785398);
			//ctx->fillUtf8Text(BLPoint(250, 80), font, rotatedText);
		}

		ctx->restore(cookie2);
		ctx->restore(cookie1);


		ctx->save(cookie1);

		ctx->translate(100, 100);

		{
			BLFont font;
			font.createFromFace(_font_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFF0000FF));
			ctx->fillUtf8Text(BLPoint(60, 80), font, regularText);

			ctx->rotate(0.785398);
			ctx->fillUtf8Text(BLPoint(250, 80), font, rotatedText);
		}
		ctx->restore(cookie1);

		//dm.end();
	}


	void paint_t1(BLContext* ctx)
	{
		//ctx->fillAll(BLRgba32(0xFF000000u));
		double _angle = 45;

		BLPath p;

		int count = 300;
		double PI = 3.14159265359;

		double cx =  _bitmap_cx/ 2.0f;
		double cy =  _bitmap_cy / 2.0f;
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
void blend2d_winapi_paint(HWND hwnd, HDC hdc)
{
	_blend2d_winapi->paint(hwnd, hdc);
}

ULONG_PTR gdiplus_token;

void blend2d_winapi_init(void)
{
	//-----------------------------------------------------------------------
	HRESULT hResult;


	hResult = ::OleInitialize(NULL);


	//-----------------------------------------------------------------------
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;


	Gdiplus::Status status;


	status = Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplusStartupInput, NULL);


	//-----------------------------------------------------------------------
	_blend2d_winapi = new blend2d_winapi();
	
	_blend2d_winapi->create();
}

void blend2d_winapi_term(void)
{
	//-----------------------------------------------------------------------
	_blend2d_winapi->destory();

	delete _blend2d_winapi;


	//-----------------------------------------------------------------------
	// Shutdown GDI+
	Gdiplus::GdiplusShutdown(gdiplus_token);


	//-----------------------------------------------------------------------
	::OleUninitialize();
}
