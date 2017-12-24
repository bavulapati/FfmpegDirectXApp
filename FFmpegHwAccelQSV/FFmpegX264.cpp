// GDI_CapturingAnImage.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "FFmpegX264.h"
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "RPCScreenCapture.h"

#include <d3d9.h>

CRITICAL_SECTION  m_critial;

IDirect3D9 *m_pDirect3D9 = NULL;
IDirect3DDevice9 *m_pDirect3DDevice = NULL;
IDirect3DSurface9 *m_pDirect3DSurfaceRender = NULL;

IDirect3DSurface9 *m_pDirect3DSurfaceRenderHost = NULL;
IDirect3DDevice9 *m_pDirect3DDeviceHost = NULL;

RECT m_rtViewport;
RECT desktop;


#define INBUF_SIZE 20480

#define MAX_LOADSTRING 100

UCHAR* CaptureBuffer = NULL;

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name

										// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


bool captured = false;
AVPacket *pkt;

void Cleanup()
{
	EnterCriticalSection(&m_critial);
	if (m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if (m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if (m_pDirect3D9)
		m_pDirect3D9->Release();

	if (m_pDirect3DSurfaceRenderHost)
		m_pDirect3DSurfaceRenderHost->Release();
	if (m_pDirect3DDeviceHost)
		m_pDirect3DDeviceHost->Release();

	LeaveCriticalSection(&m_critial);

	if (font) { font->Release(); font = 0; }
}

void AVCodecCleanup()
{
	free(is_keyframe);

	fclose(log_file);
	free(CaptureBuffer);

	avcodec_close(encodingCodecContext);
	avcodec_close(decodingCodecContext);
	avcodec_free_context(&decodingCodecContext);
	av_frame_free(&iframe);
	av_frame_free(&oframe);
	avcodec_free_context(&encodingCodecContext);
	av_frame_free(&inframe);
	av_frame_free(&outframe);
	av_packet_free(&pkt);

	Cleanup();

	exit(0);
}



int InitD3D(HWND hwnd, unsigned long lWidth, unsigned long lHeight)
{
	HRESULT lRet;
	InitializeCriticalSection(&m_critial);
	Cleanup();

	m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pDirect3D9 == NULL)
		return -1;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	GetClientRect(hwnd, &m_rtViewport);

	lRet = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &m_pDirect3DDevice);
	if (FAILED(lRet))
		return -1;

	/*D3DPRESENT_PARAMETERS d3dppHost;
	ZeroMemory(&d3dpp, sizeof(d3dppHost));
	d3dppHost.Windowed = TRUE;
	d3dppHost.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dppHost.BackBufferFormat = (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');

	lRet = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dppHost, &m_pDirect3DDeviceHost);
	if (FAILED(lRet))
		return -1;*/

	lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
		lWidth, lHeight,
		(D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'),
		D3DPOOL_DEFAULT,
		&m_pDirect3DSurfaceRender,
		NULL);
	if (FAILED(lRet))
		return -1;

	/*lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
		lWidth, lHeight,
		D3DFMT_R8G8B8,
		D3DPOOL_DEFAULT,
		&m_pDirect3DSurfaceRenderHost,
		NULL);
	if (FAILED(lRet))
		return -1;*/

	font = NULL;
	lRet = D3DXCreateFont(m_pDirect3DDevice, 22, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, FF_DONTCARE, "Arial", &font);
	if (!SUCCEEDED(lRet))
	{
		return -1;
	}

	SetRect(&fRectangle, 160, 160, 600, 204);
	message = "FUck the fonts";

	return 0;
}

bool RenderHost(BYTE* frame, AVFrame* oframe)
{
	HRESULT lRet;

	if (m_pDirect3DSurfaceRenderHost == NULL)
		return -1;
	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRenderHost->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return -1;

	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

	int pixel_h = oframe->height, pixel_w = stride;

	
		for (i = 0; i < pixel_h; i++) {
			for (int j = 0; j < pixel_w; j+=4)
			{
				pDest[i * stride + j * 3] = frame[i * pixel_w + j * 4 + 2];
				pDest[i * stride + j * 3 + 1] = frame[i * pixel_w + j * 4 + 1];
				pDest[i * stride + j * 3 + 2] = frame[i * pixel_w + j * 4];
			}
		}
	
	lRet = m_pDirect3DSurfaceRenderHost->UnlockRect();
	if (FAILED(lRet))
		return -1;

	if (m_pDirect3DDeviceHost == NULL)
		return -1;

	IDirect3DSurface9 * pBackBuffer = NULL;
	m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

	m_pDirect3DDeviceHost->StretchRect(m_pDirect3DSurfaceRenderHost, NULL, pBackBuffer, &desktop, D3DTEXF_LINEAR);

	lRet = m_pDirect3DSurfaceRenderHost->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return -1;

	frame = (BYTE *)d3d_rect.pBits;
	stride = d3d_rect.Pitch;
	BYTE *pY = oframe->data[0];
	BYTE *pUV = oframe->data[1];
	//BYTE *pV = oframe->data[2];

	
	memcpy(pY, frame, oframe->height*stride);
	memcpy(pUV, frame + oframe->height*stride, oframe->height / 2 * stride);

	lRet = m_pDirect3DSurfaceRenderHost->UnlockRect();
	if (FAILED(lRet))
		return -1;

	pBackBuffer->Release();


	return true;
}



bool Render(AVFrame* frame)
{
	HRESULT lRet;
	
	if (m_pDirect3DSurfaceRender == NULL)
		return -1;
	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return -1;

	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

	int pixel_h = frame->height, pixel_w = frame->linesize[0];

	//Copy Data
	if (frame->format == AV_PIX_FMT_YUV420P) 
	{
		for (i = 0; i < pixel_h; i++) {
			memcpy(pDest + i * stride, frame->data[0] + i * pixel_w, pixel_w);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(pDest + stride * pixel_h + i * stride / 2, frame->data[2] + i * pixel_w / 2, pixel_w / 2);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(pDest + stride * pixel_h + stride * pixel_h / 4 + i * stride / 2, frame->data[1] + i * pixel_w / 2, pixel_w / 2);
		}
	} 
	else if (frame->format == AV_PIX_FMT_NV12)
	{
		for (i = 0; i < pixel_h; i++) {
			memcpy(pDest + i * stride, frame->data[0] + i * pixel_w, pixel_w);
		}
		for (i = 0; i < pixel_h/2; i++) {
			memcpy(pDest + pixel_h*stride + i * stride, frame->data[1] + i * pixel_w, pixel_w);
		}
	}
	lRet = m_pDirect3DSurfaceRender->UnlockRect();
	if (FAILED(lRet))
		return -1;

	if (m_pDirect3DDevice == NULL)
		return -1;

	m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, &m_rtViewport, D3DTEXF_LINEAR);

	if (font && !textDrawn)
	{
		font->DrawTextA(NULL, message.c_str(), -1, &fRectangle, DT_LEFT, D3DCOLOR_XRGB(50, 0, 0));
		textDrawn = true;
	}

	m_pDirect3DDevice->EndScene();
	m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
	pBackBuffer->Release();

	

	return true;
}



int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FFMPEGHWACCELQSV, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT) {
		//PeekMessage, not GetMessage
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			driver(msg.hwnd);
			AVCodecCleanup();
		}
	}

	UnregisterClass(szWindowClass, hInstance);
	return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_FFMPEGHWACCELQSV));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_FFMPEGHWACCELQSV);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	int pixel_w = 1208, pixel_h = 665;

	//hwnd = CreateWindow(L"D3D", L"Simplest Video Play Direct3D (Surface)", WS_OVERLAPPEDWINDOW, 100, 100, screen_w, screen_h, NULL, NULL, hInstance, NULL);

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 0, 0, pixel_w, pixel_h, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	/*GetWindowRect(GetDesktopWindow(), &desktop);
	pixel_w = desktop.right - desktop.left;
	pixel_h = desktop.bottom - desktop.top;*/


	GetWindowRect(GetDesktopWindow(), &desktop);
	pixel_w = desktop.right - desktop.left;
	pixel_h = desktop.bottom - desktop.top;

	if (InitD3D(hWnd, pixel_w, pixel_h) == E_FAIL) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved


void save_as_bitmap(unsigned char *bitmap_data, int rowPitch, int width, int height, char *filename)
{
	// A file is created, this is where we will save the screen capture.

	FILE *f;

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	//Make the size negative if the image is upside down.
	bi.biHeight = -height;
	//There is only one plane in RGB color space where as 3 planes in YUV.
	bi.biPlanes = 1;
	//In windows RGB, 8 bit - depth for each of R, G, B and alpha.
	bi.biBitCount = 32;
	//We are not compressing the image.
	bi.biCompression = BI_RGB;
	// The size, in bytes, of the image. This may be set to zero for BI_RGB bitmaps.
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// rowPitch = the size of the row in bytes.
	DWORD dwSizeofImage = rowPitch * height;

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwSizeofImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	// TODO: Handle getting current directory
	fopen_s(&f, filename, "wb");

	DWORD dwBytesWritten = 0;
	dwBytesWritten += fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, f);
	dwBytesWritten += fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, f);
	dwBytesWritten += fwrite(bitmap_data, 1, dwSizeofImage, f);

	fclose(f);
}

//void DrawTextString(int x, int y, DWORD color, const char *str)
//{
//
//	HRESULT r = 0;
//
//
//	// Get a handle for the font to use
//
//	HFONT hFont = (HFONT)GetStockObject(SYSTEM_FONT);
//	D3DXFONT pFont = 0;
//
//	// Create the D3DX Font
//	r = D3DXCreateFont(g_pd3dDevice, hFont, &pFont);
//
//	if (FAILED(r))
//		//Do Debugging
//
//		// Rectangle where the text will be located
//		RECT TextRect = { x,y,0,0 };
//
//	// Inform font it is about to be used
//	pFont->Begin();
//
//	// Calculate the rectangle the text will occupy
//	pFont->DrawText(str, -1, &TextRect, DT_CALCRECT, 0);
//
//	// Output the text, left aligned
//	pFont->DrawText(str, -1, &TextRect, DT_LEFT, color);
//
//	// Finish up drawing
//	pFont->End();
//
//
//	// Release the font
//	pFont->Release();
//
//}

void decode(AVPacket *pkt)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(decodingCodecContext, pkt);
	if (ret < 0) 
	{
		fprintf(log_file, "Error sending a packet for decoding\n");
		AVCodecCleanup();
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decodingCodecContext, iframe);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(log_file, "Error during decoding\n");
			AVCodecCleanup();
		}

		//printf("saving frame %3d\n", dc->frame_number);
		fflush(log_file);

		/* the picture is allocated by the decoder. no need to free it */
		_snprintf_s(buf, sizeof(buf), "%s\\Decoded\\%d.bmp", pwd, decodingCodecContext->frame_number);

		is_keyframe[decodingCodecContext->frame_number-1] = av_get_picture_type_char(iframe->pict_type);
		if (iframe->pict_type == AV_PICTURE_TYPE_I)
		{
			number_of_I_frames++;
			fprintf(log_file, "I - frame index : %d\n", decodingCodecContext->frame_number);
		}
		else if (iframe->pict_type == AV_PICTURE_TYPE_P)
			number_of_P_frames++;

		Render(iframe);

		
		_snprintf_s(buf, sizeof(buf), "%s\\Decoded\\_%d.bmp", pwd, decodingCodecContext->frame_number);
		//save_as_bitmap(oframe->data[0], oframe->linesize[0], oframe->width, oframe->height, buf);
	}
}

int decode_packet(DecodeContext *decodeContext, AVCodecContext *decoder_ctx,
	AVFrame *frame, AVFrame *sw_frame,
	AVPacket *pkt)
{
	char file_name[MAX_PATH];
	int ret = 0;

	ret = avcodec_send_packet(decoder_ctx, pkt);
	if (ret < 0) {
		fprintf(log_file, "Error during decoding\n");
		return ret;
	}

	while (ret >= 0) {
		int i, j;

		ret = avcodec_receive_frame(decoder_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			fprintf(log_file, "Error during decoding\n");
			return ret;
		}

		/* A real program would do something useful with the decoded frame here.
		* We just retrieve the raw data and write it to a file, which is rather
		* useless but pedagogic. */
		ret = av_hwframe_transfer_data(sw_frame, frame, 0);
		if (ret < 0) {
			fprintf(log_file, "Error transferring the data to system memory\n");
			goto fail;
		}

		Render(sw_frame);

		/*int scale_rc = sws_scale(yuv_to_rgb_SwsContext
			, sw_frame->data, sw_frame->linesize
			, 0, sw_frame->height
			, oframe->data, oframe->linesize);

		snprintf(file_name, sizeof(file_name), "%s\\Decoded\\%d.bmp", "C:\\Bala", decoder_ctx->frame_number);

		save_as_bitmap(oframe->data[0], oframe->linesize[0], oframe->width, oframe->height, file_name);*/

		/*for (i = 0; i < FF_ARRAY_ELEMS(sw_frame->data) && sw_frame->data[i]; i++)
		for (j = 0; j < (sw_frame->height >> (i > 0)); j++)
		avio_write(output_ctx, sw_frame->data[i] + j * sw_frame->linesize[i], sw_frame->width);*/

	fail:
		av_frame_unref(sw_frame);
		av_frame_unref(frame);

		if (ret < 0)
			return ret;
	}

	return 0;
}

void encode(AVFrame *frame, AVPacket *pkt)
{
	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %d\n", frame->pts);

	ret = avcodec_send_frame(encodingCodecContext, frame);
	if (ret < 0) {
		fprintf(log_file, "Error sending a frame for encoding\n");
		AVCodecCleanup();
	}
	
	while (ret >= 0) 
	{
		ret = avcodec_receive_packet(encodingCodecContext, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
			
		else if (ret < 0) {
			fprintf(log_file, "Error during encoding\n");
			AVCodecCleanup();
		}

		//printf("Write packet %d (size=%5d)\n", pkt->pts, pkt->size);
		
		start_decode = clock();
		ret = decode_packet(&decodeContext, decodingCodecContext, iframe, sw_frame, pkt);
		stop_decode = clock();
		nDiff_decode = stop_decode - start_decode;
		time_for_decode += nDiff_decode;
		number_of_packets_decoded++;
		//fprintf(log_file, "time for decode - %d\n", nDiff);

		//fprintf(log_file, "size of packet- %d\n", pkt->size);
		total_size_of_packets += pkt->size;
		number_of_packets++;
		//fwrite(pkt->data, 1, pkt->size, h264_file);
		av_packet_unref(pkt);

	}
}

enum AVPixelFormat get_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
	while (*pix_fmts != AV_PIX_FMT_NONE) {
		if (*pix_fmts == AV_PIX_FMT_QSV) {
			DecodeContext *decode = (DecodeContext *)avctx->opaque;
			AVHWFramesContext  *frames_ctx;
			AVQSVFramesContext *frames_hwctx;
			int ret;

			/* create a pool of surfaces to be used by the decoder */
			avctx->hw_frames_ctx = av_hwframe_ctx_alloc(decode->hw_device_ref);
			if (!avctx->hw_frames_ctx)
				return AV_PIX_FMT_NONE;
			frames_ctx = (AVHWFramesContext*)avctx->hw_frames_ctx->data;
			frames_hwctx = (AVQSVFramesContext *)frames_ctx->hwctx;

			frames_ctx->format = AV_PIX_FMT_QSV;
			frames_ctx->sw_format = avctx->sw_pix_fmt;
			frames_ctx->width = FFALIGN(avctx->coded_width, 32);
			frames_ctx->height = FFALIGN(avctx->coded_height, 32);
			frames_ctx->initial_pool_size = 32;

			frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

			ret = av_hwframe_ctx_init(avctx->hw_frames_ctx);
			if (ret < 0)
				return AV_PIX_FMT_NONE;

			return AV_PIX_FMT_QSV;
		}

		pix_fmts++;
	}

	fprintf(log_file, "The QSV pixel format not offered in get_format()\n");

	return AV_PIX_FMT_NONE;
}

void createDirectory(const char * folder_name)
{
	CreateDirectory(folder_name, NULL);
	if (ERROR_ALREADY_EXISTS == GetLastError())
	{
		fprintf(log_file, "The Captured directory already exists.\n");
	}
	else
	{
		fprintf(log_file, "The Captured directory could not be created.\n");
	}
}

void driver(HWND hWnd)
{
	const char *filename, *codec_name, *folder_name;
	const AVCodec *codec, *decodec;
	int i, ret, x, y;
	
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	fopen_s(&log_file, "C:\\Bala\\log.txt", "w");

	GetCurrentDirectory(MAX_PATH, pwd);


	folder_name = "Captured";

	//createDirectory(folder_name);

	folder_name = "Decoded";

	//createDirectory(folder_name);
	
	filename = "C:\\Bala\\encoded.h264";
	//const char *outfilename = "decoded.avi";
	codec_name = "libx264";
	//MessageBox(hWnd, (LPCWSTR)"1", (LPCWSTR)"1", 1);
	avcodec_register_all();

	/* find the video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		fprintf(log_file, "Codec '%s' not found\n", codec_name);
		AVCodecCleanup();
	}	

	
	fprintf(log_file, "name - %s, help - %s, default_val - %s\n", codec->priv_class->option->name, codec->priv_class->option->help, codec->priv_class->option->default_val);
	

	encodingCodecContext = avcodec_alloc_context3(codec);
	if (!encodingCodecContext) {
		fprintf(log_file, "Could not allocate video codec context\n");
		AVCodecCleanup();
	}
	
	pkt = av_packet_alloc();
	if (!pkt)
		AVCodecCleanup();

	/* put sample parameters */
	encodingCodecContext->bit_rate = 4000000;
	/* resolution must be a multiple of two */
	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(NULL, &rcClient);

	int width = 1600, height = 900;

	
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);
	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	width = desktop.right;
	height = desktop.bottom;

	// We always want the input to encode as 
	// a frame with a fixed resolution 
	// irrespective of screen resolution.
	encodingCodecContext->width = width;// 1024;
	encodingCodecContext->height = height;//512;
	
	/* frames per second */
	encodingCodecContext->time_base.num =  1;
	encodingCodecContext->time_base.den = 60;
	encodingCodecContext->framerate.num = 60;
	encodingCodecContext->framerate.den = 1;


	encodingCodecContext->pix_fmt = AV_PIX_FMT_NV12;
	

	if (codec->id == AV_CODEC_ID_H264)
	{
		av_opt_set(encodingCodecContext->priv_data, "preset", "veryfast", 0);
		av_opt_set(encodingCodecContext->priv_data, "tune", "zerolatency", 0);
		av_opt_set(encodingCodecContext->priv_data, "crf", "23", 0);
		
	}

	/* open it */
	ret = avcodec_open2(encodingCodecContext, codec, NULL);
	if (ret < 0) {
		printf("Could not open codec: \n");
		AVCodecCleanup();
	}	


	fopen_s(&h264_file, filename, "wb");
	if (!h264_file) {
		fprintf(log_file, "Could not open %s\n", filename);
		AVCodecCleanup();
	}

	/*fopen_s(&outfile, outfilename, "wb");
	if (!outfile) {
		fprintf(log_file, "Could not open %s\n", outfilename);
		exit(1);
	}*/

	outframe = av_frame_alloc();
	if (!outframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}
	outframe->format = AV_PIX_FMT_NV12;
	outframe->width = encodingCodecContext->width;
	outframe->height = encodingCodecContext->height;



	ret = av_frame_get_buffer(outframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}


	inframe = av_frame_alloc();
	if (!inframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}
	inframe->format = AV_PIX_FMT_RGB32;
	inframe->width = width;
	inframe->height = height;

	ret = av_frame_get_buffer(inframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}

	iframe = av_frame_alloc();
	if (!iframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}

	iframe->format = AV_PIX_FMT_NV12;
	iframe->width = encodingCodecContext->width;
	iframe->height = encodingCodecContext->height;

	ret = av_frame_get_buffer(iframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}

	oframe = av_frame_alloc();
	if (!oframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}

	oframe->format = AV_PIX_FMT_RGB32;
	oframe->width = (width + 7) / 8 * 8;
	oframe->height = height;

	ret = av_frame_get_buffer(oframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}

	/* initialize the decoder */
	decodec = avcodec_find_decoder_by_name("h264_qsv");
	if (!decodec) {
		fprintf(log_file, "The QSV decoder is not present in libavcodec\n");
		AVCodecCleanup();
	}

	//decodec->priv_class->option->default_val.i64 = 1;
	//av_opt_set_int(decodec->priv_class, "async_depth", 1, 0);
	fprintf(log_file, "name - %s, help - %s, default_val - %d\n", decodec->priv_class->option->name, decodec->priv_class->option->help, decodec->priv_class->option->default_val.i64);

	decodingCodecContext = avcodec_alloc_context3(decodec);
	if (!decodingCodecContext) {
		ret = AVERROR(ENOMEM);
		AVCodecCleanup();
	}
	//av_opt_set_int(decodingCodecContext->priv_data, "async_depth", 1, 0);

	decodingCodecContext->codec_id = AV_CODEC_ID_H264;
	//decoder_ctx->refcounted_frames = 1;
	decodingCodecContext->opaque = &decodeContext;
	decodingCodecContext->get_format = get_format;

	/* open the hardware device */
	ret = av_hwdevice_ctx_create(&decodeContext.hw_device_ref, AV_HWDEVICE_TYPE_QSV,
		"auto_any", NULL, 0);
	if (ret < 0) {
		fprintf(log_file, "Cannot open the hardware device\n");
		char buf[1024];
		av_strerror(ret, buf, sizeof(buf));
		fprintf(log_file, "%s\n", buf);
		AVCodecCleanup();
	}


	/* open it */
	ret = avcodec_open2(decodingCodecContext, NULL, NULL);
	if (ret < 0) {
		fprintf(log_file, "Error opening the output context: \n");
		AVCodecCleanup();
	}

	frame = av_frame_alloc();
	sw_frame = av_frame_alloc();
	if (!frame || !sw_frame) {
		ret = AVERROR(ENOMEM);
		fprintf(log_file, "Error allocating the frames: \n");
		AVCodecCleanup();
	}


	//Capture flow is here
	
	long CaptureLineSize = NULL;
	ScreenCaptureProcessorGDI *D3D11 = new ScreenCaptureProcessorGDI();

	//Capture part ends here.

	D3D11->init();


	CaptureBuffer = (UCHAR*)malloc(inframe->height*inframe->linesize[0]);

	rgb_to_yuv_SwsContext = sws_getContext(inframe->width, inframe->height, AV_PIX_FMT_RGB32,
		outframe->width, outframe->height, AV_PIX_FMT_NV12,
		SWS_BICUBIC, NULL, NULL, NULL);
	/* create scaling context */

	if (!rgb_to_yuv_SwsContext) {
		fprintf(log_file,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name(AV_PIX_FMT_RGB32), inframe->width, inframe->height,
			av_get_pix_fmt_name(AV_PIX_FMT_NV12), outframe->width, outframe->height);
		ret = AVERROR(EINVAL);
		AVCodecCleanup();
	}

	/* create scaling context */
	yuv_to_rgb_SwsContext = sws_getContext(iframe->width, iframe->height, (AVPixelFormat)iframe->format,
		oframe->width, oframe->height, AV_PIX_FMT_RGB32,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!yuv_to_rgb_SwsContext) {
		fprintf(log_file,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name((AVPixelFormat)iframe->format), iframe->width, iframe->height, av_get_pix_fmt_name(AV_PIX_FMT_RGB32), oframe->width, oframe->height);
		ret = AVERROR(EINVAL);
		AVCodecCleanup();
	}

	char buf[1024];

	int number_of_frames_to_process = 100;
	is_keyframe = (char *)malloc(number_of_frames_to_process + 10);

	for (i = 0; i < number_of_frames_to_process; i++) 
	{
		fflush(stdout);
		//Sleep(500);
		/* make sure the frame data is writable */
		/*ret = av_frame_make_writable(inframe);
		if (ret < 0)
			exit(1);*/

		char * srcData = NULL;
		//SleepEx(5, true);

	
		start = clock();
		D3D11->GrabImage(CaptureBuffer, CaptureLineSize);
		stop = clock();


		nDiff = stop - start;
		time_for_capture += nDiff;
		number_of_frames_captured++;

		//fprintf(file, "time for capture - %d\n", nDiff);

		uint8_t * inData[1] = { (uint8_t *)CaptureBuffer }; // RGB have one plane
		int inLinesize[1] = { CaptureLineSize /*av_image_get_linesize(AV_PIX_FMT_RGB32, inframe->width, 0)*//*inframe->width * 4*/ }; // RGB stride
		
		start = clock();
		
		//RenderHost(CaptureBuffer, outframe);

		int scale_rc = sws_scale(rgb_to_yuv_SwsContext, inData, inLinesize, 0, inframe->height, outframe->data,
			outframe->linesize);

		
		stop = clock();
		nDiff = stop - start;
		//fprintf(file, "time for scale - %d\n", nDiff);
		time_for_scaling_rgb_to_yuv += nDiff;
		number_of_rgb_frames_scaled++;

		//char * filename24 = "captured";
		//snprintf(buf, sizeof(buf), "E:\\h264-24\\%s-%d.bmp", filename24, i);

		//pgm_save(inData[0], inframe->width, inframe->height, buf);

		//outframe->data[0] = (uint8_t *)pgm_read24( outframe->width, outframe->height, buf);

		outframe->pts = i;



		/* encode the image */
		start = clock();
		encode(outframe, pkt);
		stop = clock();
		nDiff = stop - start;
		time_for_encode += nDiff;
		number_of_frames_encoded++;
		//fprintf(file, "time for encode - %d\n", nDiff);

		/* decode the image */
		/*if (pkt->size != 0)
		{
			start = clock();
			decode(pkt, "decoded");
			stop = clock();
			nDiff = stop - start;
			tdec += nDiff;
			ndec++;
			fprintf(file, "time for decode - %d\n", nDiff);
			av_packet_unref(pkt);
		}*/

	}

	/* flush the encoder */
	start = clock();
	encode(NULL, pkt);
	stop = clock();
	nDiff = stop - start;
	time_for_encode += nDiff;
	number_of_frames_encoded++;

	/* flush the decoder */
	start_decode = clock();
	ret = decode_packet(&decodeContext, decodingCodecContext, iframe, sw_frame, NULL);
	stop_decode = clock();
	nDiff_decode = stop_decode - start_decode;
	time_for_decode += nDiff_decode;
	number_of_packets_decoded++;

	/* add sequence end code to have a real MPEG file */
	fwrite(endcode, 1, sizeof(endcode), h264_file);
	fclose(h264_file);

	//fwrite(endcode, 1, sizeof(endcode), outfile);
	//fclose(outfile);

	fprintf(log_file, "average time for scale - %f\n", time_for_scaling_rgb_to_yuv/ (float) number_of_rgb_frames_scaled);
	fprintf(log_file, "average time for encode - %f\n", time_for_encode/ (float) number_of_frames_encoded);
	fprintf(log_file, "average time for decode - %f\n", time_for_decode/(float) number_of_packets_decoded);
	fprintf(log_file, "average time for capture - %f\n", time_for_capture/ (float) number_of_frames_captured);
	fprintf(log_file, "average size after encoding - %f\n", total_size_of_packets / (float)number_of_packets);
	is_keyframe[decodingCodecContext->frame_number] = '\0';
	fwrite(is_keyframe, 1, decodingCodecContext->frame_number+1, log_file);
	fprintf(log_file, "\n", NULL);
	fprintf(log_file, "Number of I - frames : %d\n", number_of_I_frames);
	fprintf(log_file, "Number of P - frames : %d\n", number_of_packets);
	
	AVCodecCleanup();
}



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparma, LPARAM lparam)
{
	switch (msg) {
	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparma, lparam);
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
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