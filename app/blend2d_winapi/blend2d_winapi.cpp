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


class duration_measurer
{
public:
	std::chrono::system_clock::time_point _start;
	std::string _name;

public:
	void start(std::string name)
	{
		_start = std::chrono::system_clock::now();
		_name = name;
	}

	void end(void)
	{
		std::chrono::system_clock::time_point _end;


		_end = std::chrono::system_clock::now();


		std::chrono::microseconds _duration;


		_duration = std::chrono::duration_cast<std::chrono::microseconds>(_end - _start);


		std::ostringstream _oss;


		_oss << _name << " duration: " << _duration.count() << "usec" << std::endl;


		OutputDebugStringA(_oss.str().c_str());
	}
};




/////////////////////////////////////////////////////////////////////////////
//===========================================================================
class blend2d_winapi
{
public:
	void paint(HWND hwnd, HDC hdc)
	{
		paint_blend2d();


		duration_measurer dm;

		dm.start("Gdiplus");

		Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromHWND(hwnd, FALSE);

#if 0
		pGraphics->SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		pGraphics->SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
		pGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
		pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
		pGraphics->SetInterpolationMode(Gdiplus::InterpolationModeDefault);
#endif

		pGraphics->DrawImage(_bitmap, 0, 0);

		delete pGraphics;

		dm.end();
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

	void create_bitmap()
	{
		_bitmap_data_size = _bitmap_cx * _bitmap_cy * 4;

		BITMAPINFO bmi;


		memset(&bmi, 0, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = static_cast<LONG>(_bitmap_cx);
		bmi.bmiHeader.biHeight = -static_cast<LONG>(_bitmap_cy);
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biBitCount = 32;

		_bitmap_data = new std::uint8_t[_bitmap_data_size];

		_bitmap = new Gdiplus::Bitmap(&bmi, _bitmap_data);

		memset(_bitmap_data, 0xA0, _bitmap_data_size);
	}

	void destroy_bitmap()
	{
		delete _bitmap;
		delete[] _bitmap_data;
	}

public:
	BLImage _blimg;
	BLContext _blctx;

	void create_blend2d()
	{
		init_font();


		//_blimg = BLImage(static_cast<int>(_bitmap_cx), static_cast<int>(_bitmap_cy), BL_FORMAT_PRGB32);
		_blimg.createFromData(_bitmap_cx, _bitmap_cy, BL_FORMAT_PRGB32, _bitmap_data, _bitmap_cx*4);


		_blctx = BLContext(_blimg);
	}

	void destroy_blend2d()
	{
		//delete _blctx;
		//delete _blimg;
	}

	void paint_blend2d()
	{
		duration_measurer dm;

		dm.start("blend2d");


		_blctx.clearAll();
		paint_ex(&_blctx);
		_blctx.end();


		dm.end();
	}

	void paint_ex(BLContext* ctx)
	{
		paint_ex5(ctx);
		paint_ex7(ctx);
		paint_t1(ctx);
	}

	void paint_ex5(BLContext* ctx)
	{
		duration_measurer dm;

		dm.start("ex5");


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

		dm.end();
	}

	BLFontFace _face;
	int init_font(void)
	{
		BLResult result = _face.createFromFile("C:/Windows/Fonts/malgun.ttf");
		if (result != BL_SUCCESS) {
			printf("Failed to load a font (err=%u)\n", result);
			return 1;
		}

	}

	void paint_ex7(BLContext* ctx)
	{
		duration_measurer dm;

		dm.start("ex7");


		const char regularText[] = "Hello Blend2D!";
		const char rotatedText[] = u8"Rotated Text ÇÑ±Û";

		BLContextCookie cookie1;
		BLContextCookie cookie2;

		ctx->save(cookie1);
		ctx->scale(0.5);
		{
			BLFont font;
			font.createFromFace(_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFFFF0000));
			ctx->fillUtf8Text(BLPoint(60, 80), font, regularText);

			//ctx->rotate(0.785398);
			//ctx->fillUtf8Text(BLPoint(250, 80), font, rotatedText);
		}

		ctx->save(cookie2);
		ctx->scale(0.5);
		{

			BLFont font;
			font.createFromFace(_face, 50.0f);

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
			font.createFromFace(_face, 50.0f);

			ctx->setFillStyle(BLRgba32(0xFF0000FF));
			ctx->fillUtf8Text(BLPoint(60, 80), font, regularText);

			ctx->rotate(0.785398);
			ctx->fillUtf8Text(BLPoint(250, 80), font, rotatedText);
		}
		ctx->restore(cookie1);

		dm.end();
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
