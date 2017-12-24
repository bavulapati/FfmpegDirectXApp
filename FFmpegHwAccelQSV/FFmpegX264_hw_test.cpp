// GDI_CapturingAnImage.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "FFmpegX264.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "RPCScreenCapture.h"






#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>

AVBufferRef *hw_device_ctx = NULL;
enum AVPixelFormat hw_pix_fmt;
FILE *output_file = NULL;



#define INBUF_SIZE 20480

#define MAX_LOADSTRING 100


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


int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FFMPEGHWACCELQSV, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FFMPEGHWACCELQSV));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
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

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 1008, 465, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
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
	f = fopen(filename, "wb");

	DWORD dwBytesWritten = 0;
	dwBytesWritten += fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, f);
	dwBytesWritten += fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, f);
	dwBytesWritten += fwrite(bitmap_data, 1, dwSizeofImage, f);

	fclose(f);
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
		
		int scale_rc = sws_scale(nv12_to_rgb_SwsContext
			, sw_frame->data, sw_frame->linesize
			, 0, sw_frame->height
			, oframe->data, oframe->linesize);

		_snprintf(file_name, sizeof(file_name), "%s\\Decoded\\%d.bmp", pwd, decoder_ctx->frame_number);

		save_as_bitmap(oframe->data[0], oframe->linesize[0], oframe->width, oframe->height, file_name);

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


//enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
//	const enum AVPixelFormat *pix_fmts)
//{
//	const enum AVPixelFormat *p;
//
//	for (p = pix_fmts; *p != -1; p++) {
//		if (*p == hw_pix_fmt)
//			return *p;
//	}
//
//	fprintf(stderr, "Failed to get HW surface format.\n");
//	return AV_PIX_FMT_NONE;
//}

enum AVPixelFormat get_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
	while (*pix_fmts != AV_PIX_FMT_NONE) {
		if (*pix_fmts == AV_PIX_FMT_QSV) {
			DecodeContext *decode = (DecodeContext *) avctx->opaque;
			AVHWFramesContext  *frames_ctx;
			AVQSVFramesContext *frames_hwctx;
			int ret;

			/* create a pool of surfaces to be used by the decoder */
			avctx->hw_frames_ctx = av_hwframe_ctx_alloc(decode->hw_device_ref);
			if (!avctx->hw_frames_ctx)
				return AV_PIX_FMT_NONE;
			frames_ctx = (AVHWFramesContext*)avctx->hw_frames_ctx->data;
			frames_hwctx = (AVQSVFramesContext *) frames_ctx->hwctx;

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

void encode(AVFrame *frame, AVPacket *pkt)
{

	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %d\n", frame->pts);

	ret = avcodec_send_frame(encoder_ctx, frame);
	if (ret < 0) {
		fprintf(log_file, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) 
	{
		ret = avcodec_receive_packet(encoder_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
			
		else if (ret < 0) {
			fprintf(log_file, "Error during encoding\n");
			exit(1);
		}

		//printf("Write packet %d (size=%5d)\n", pkt->pts, pkt->size);
		
		/*start_decode = clock();
		ret = decode_packet(&decodeContext, decoder_ctx, iframe, sw_frame, pkt);
		stop_decode = clock();
		nDiff_decode = stop_decode - start_decode;
		time_for_decode += nDiff_decode;
		number_of_packets_decoded++;*/
		//fprintf(log_file, "time for decode - %d\n", nDiff);

		//fprintf(log_file, "size of packet- %d\n", pkt->size);
		total_size_of_packets += pkt->size;
		number_of_packets++;
		//fwrite(pkt->data, 1, pkt->size, h264_file);
		av_packet_unref(pkt);

	}
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


/*
* Copyright (c) 2017 Jun Zhao
* Copyright (c) 2017 Kaixuan Liu
*
* HW Acceleration API (video decoding) decode sample
*
* This file is part of FFmpeg.
*
* FFmpeg is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* FFmpeg is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/**
* @file
* HW-Accelerated decoding example.
*
* @example hw_decode.c
* This example shows how to do HW-accelerated decoding with output
* frames from the HW video surfaces.
*/



static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
	enum AVPixelFormat fmt;

	switch (type) {
	case AV_HWDEVICE_TYPE_VAAPI:
		fmt = AV_PIX_FMT_VAAPI;
		break;
	case AV_HWDEVICE_TYPE_DXVA2:
		fmt = AV_PIX_FMT_DXVA2_VLD;
		break;
	case AV_HWDEVICE_TYPE_D3D11VA:
		fmt = AV_PIX_FMT_D3D11;
		break;
	case AV_HWDEVICE_TYPE_VDPAU:
		fmt = AV_PIX_FMT_VDPAU;
		break;
	case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
		fmt = AV_PIX_FMT_VIDEOTOOLBOX;
		break;
	default:
		fmt = AV_PIX_FMT_NONE;
		break;
	}

	return fmt;
}

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
		NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
	const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

static int decode_write(AVCodecContext *avctx, AVPacket *packet)
{
	AVFrame *frame = NULL, *sw_frame = NULL;
	AVFrame *tmp_frame = NULL;
	uint8_t *buffer = NULL;
	int size;
	int ret = 0;

	ret = avcodec_send_packet(avctx, packet);
	if (ret < 0) {
		fprintf(stderr, "Error during decoding\n");
		return ret;
	}

	while (ret >= 0) {
		if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
			fprintf(stderr, "Can not alloc frame\n");
			ret = AVERROR(ENOMEM);
			goto fail;
		}

		ret = avcodec_receive_frame(avctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_frame_free(&frame);
			av_frame_free(&sw_frame);
			return 0;
		}
		else if (ret < 0) {
			fprintf(stderr, "Error while decoding\n");
			goto fail;
		}

		if (frame->format == hw_pix_fmt) {
			/* retrieve data from GPU to CPU */
			if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
				fprintf(stderr, "Error transferring the data to system memory\n");
				goto fail;
			}
			tmp_frame = sw_frame;
		}
		else
			tmp_frame = frame;

		size = av_image_get_buffer_size((AVPixelFormat)tmp_frame->format, tmp_frame->width,
			tmp_frame->height, 1);
		buffer = (uint8_t *)av_malloc(size);
		if (!buffer) {
			fprintf(stderr, "Can not alloc buffer\n");
			ret = AVERROR(ENOMEM);
			goto fail;
		}
		ret = av_image_copy_to_buffer(buffer, size,
			(const uint8_t * const *)tmp_frame->data,
			(const int *)tmp_frame->linesize, (AVPixelFormat)tmp_frame->format,
			tmp_frame->width, tmp_frame->height, 1);
		if (ret < 0) {
			fprintf(stderr, "Can not copy image to buffer\n");
			goto fail;
		}

		if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
			fprintf(stderr, "Failed to dump raw data.\n");
			goto fail;
		}

	fail:
		av_frame_free(&frame);
		av_frame_free(&sw_frame);
		if (buffer)
			av_freep(&buffer);
		if (ret < 0)
			return ret;
	}

	return 0;
}

void driver(HWND hWnd)
{
	AVFormatContext *input_ctx = NULL;
	int video_stream, ret;
	AVStream *video = NULL;
	AVCodecContext *decoder_ctx = NULL;
	AVCodec *decoder = NULL;
	AVPacket packet;
	enum AVHWDeviceType type;


	av_register_all();

	type = av_hwdevice_find_type_by_name("dxva2");
	hw_pix_fmt = find_fmt_by_hw_type(type);
	if (hw_pix_fmt == -1) {
		fprintf(log_file, "Cannot support dxva2 in this example.\n");
		exit(1);
	}

	/* open the input file */
	if (avformat_open_input(&input_ctx, "dandu.mp4", NULL, NULL) != 0) {
		fprintf(log_file, "Cannot open input file Captain.mp4 \n");
		exit(1);
	}

	if (avformat_find_stream_info(input_ctx, NULL) < 0) {
		fprintf(log_file, "Cannot find input stream information.\n");
		exit(1);
	}

	/* find the video stream information */
	ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
	if (ret < 0) {
		fprintf(log_file, "Cannot find a video stream in the input file\n");
		exit(1);
	}
	video_stream = ret;

	if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
	{
		fprintf(log_file, " error AVERROR(ENOMEM) \n");
		exit(1);
	}

	video = input_ctx->streams[video_stream];
	if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
	{
		fprintf(log_file, " avcodec_parameters_to_context \n");
		exit(1);
	}

	decoder_ctx->get_format = get_hw_format;
	av_opt_set_int(decoder_ctx, "refcounted_frames", 1, 0);

	if (hw_decoder_init(decoder_ctx, type) < 0)
	{
		fprintf(log_file, " hw_decoder_init \n");
		exit(1);
	}

	if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
		fprintf(log_file, "Failed to open codec for stream #%u\n", video_stream);
		exit(1);
	}

	/* open the file to dump raw data */
	output_file = fopen("output.mov", "w");

	/* actual decoding and dump the raw data */
	while (ret >= 0) {
		if ((ret = av_read_frame(input_ctx, &packet)) < 0)
			break;

		if (video_stream == packet.stream_index)
			ret = decode_write(decoder_ctx, &packet);

		av_packet_unref(&packet);
	}

	/* flush the decoder */
	packet.data = NULL;
	packet.size = 0;
	ret = decode_write(decoder_ctx, &packet);
	av_packet_unref(&packet);

	if (output_file)
		fclose(output_file);
	avcodec_free_context(&decoder_ctx);
	avformat_close_input(&input_ctx);
	av_buffer_unref(&hw_device_ctx);
	
	return;
}

//void driver(HWND hWnd)
//{
//	AVFormatContext *input_ctx = NULL;
//	enum AVHWDeviceType type;
//
//	const char *filename, *codec_name, *folder_name;
//	const AVCodec *codec, *decoder;
//	int i, ret, x, y;
//	AVPacket *pkt;
//	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
//	TCHAR  infoBuf[INFO_BUFFER_SIZE];
//	DWORD  bufCharCount = INFO_BUFFER_SIZE;
//	GetComputerName( infoBuf, &bufCharCount );
//	fprintf(log_file, "%s\n", infoBuf);
//
//	GetCurrentDirectory(MAX_PATH, pwd);
//
//
//	folder_name = "Captured";
//
//	//createDirectory(folder_name);
//
//	folder_name = "Decoded";
//
//	//createDirectory(folder_name);
//	
//	filename = "encoded.h264";
//	//const char *outfilename = "decoded.avi";
//	codec_name = "h264_qsv";
//	avcodec_register_all();
//
//	type = av_hwdevice_find_type_by_name("dxva2");
//	hw_pix_fmt = find_fmt_by_hw_type(type);
//	if (hw_pix_fmt == -1) {
//		fprintf(log_file, "Cannot support dxva2 in this example.\n");
//		exit(1);
//	}
//
//
//	/* find the video encoder */
//	codec = avcodec_find_encoder_by_name(codec_name);
//	if (!codec) {
//		fprintf(log_file, "Codec '%s' not found\n", codec_name);
//		exit(1);
//	}	
//
//	encoder_ctx = avcodec_alloc_context3(codec);
//	if (!encoder_ctx) {
//		fprintf(log_file, "Could not allocate video codec context\n");
//		exit(1);
//	}
//
//
//	encoder_ctx->opaque = &encodeContext;
//
//	/* open the hardware device */
//	ret = av_hwdevice_ctx_create(&encodeContext.hw_device_ref, AV_HWDEVICE_TYPE_QSV,
//		"auto", NULL, 0);
//	if (ret < 0) {
//		fprintf(log_file, "Cannot open the hardware device\n");
//		char buf[1024];
//		av_strerror(ret, buf, sizeof(buf));
//		fprintf(log_file, "%s\n", buf);
//		exit(1);
//	}
//	
//	pkt = av_packet_alloc();
//	if (!pkt)
//		exit(1);
//
//	/* put sample parameters */
//	encoder_ctx->bit_rate = 4000000;
//	/* resolution must be a multiple of two */
//	// Get the client area for size calculation
//	RECT rcClient;
//	GetClientRect(NULL, &rcClient);
//
//	int width = 1366, height = 768;
//
//	RECT desktop;
//	// Get a handle to the desktop window
//	const HWND hDesktop = GetDesktopWindow();
//	// Get the size of screen to the variable desktop
//	GetWindowRect(hDesktop, &desktop);
//	// The top left corner will have coordinates (0,0)
//	// and the bottom right corner will have coordinates
//	// (horizontal, vertical)
//	width = desktop.right;
//	height = desktop.bottom;
//
//	// We always want the input to encode as 
//	// a frame with a fixed resolution 
//	// irrespective of screen resolution.
//	encoder_ctx->width = width;
//	encoder_ctx->height = height;
//	
//	/* frames per second */
//	encoder_ctx->time_base.num =  1;
//	encoder_ctx->time_base.den = 60;
//	encoder_ctx->framerate.num = 60;
//	encoder_ctx->framerate.den = 1;
//
//
//	//encoder_ctx->pix_fmt = codec->pix_fmts[0];
//	encoder_ctx->get_format = get_hw_format;
//	
//	if (hw_decoder_init(decoder_ctx, type) < 0)
//	{
//		fprintf(log_file, "Could not initialize hw encoder: \n");
//		exit(1);
//	}
//	
//
//	if (codec->id == AV_CODEC_ID_H264)
//	{
//		av_opt_set(encoder_ctx->priv_data, "preset", "veryfast", 0);
//		av_opt_set(encoder_ctx->priv_data, "tune", "zerolatency", 0);
//		av_opt_set(encoder_ctx->priv_data, "crf", "23", 0);
//	}
//
//	/* open it */
//	ret = avcodec_open2(encoder_ctx, NULL, NULL);
//	if (ret < 0) {
//		fprintf(log_file, "Could not open codec: \n");
//		exit(1);
//	}	
//
//
//	/*fopen_s(&h264_file, filename, "wb");
//	if (!h264_file) {
//		fprintf(log_file, "Could not open %s\n", filename);
//		exit(1);
//	}*/
//
//	oframe = av_frame_alloc();
//	if (!oframe) {
//		fprintf(log_file, "Could not allocate video frame\n");
//		exit(1);
//	}
//	oframe->format = AV_PIX_FMT_RGB32;
//	oframe->width = width;
//	oframe->height = height;
//
//	ret = av_frame_get_buffer(oframe, 8);
//	if (ret < 0) {
//		fprintf(log_file, "Could not allocate the video frame data\n");
//		exit(1);
//	}
//
//	iframe = av_frame_alloc();
//	if (!iframe) {
//		fprintf(log_file, "Could not allocate video frame\n");
//		exit(1);
//	}
//	iframe->format = codec->pix_fmts[0];;
//	iframe->width = encoder_ctx->width;
//	iframe->height = encoder_ctx->height;
//	
//	ret = av_frame_get_buffer(iframe, 32);
//	if (ret < 0) {
//		fprintf(log_file, "Could not allocate the video frame data\n");
//		exit(1);
//	}
//
//	outframe = av_frame_alloc();
//	if (!outframe) {
//		fprintf(log_file, "Could not allocate video frame\n");
//		exit(1);
//	}
//	outframe->format = codec->pix_fmts[0];;
//	outframe->width = encoder_ctx->width;
//	outframe->height = encoder_ctx->height;
//
//
//
//	ret = av_frame_get_buffer(outframe, 32);
//	if (ret < 0) {
//		fprintf(log_file, "Could not allocate the video frame data\n");
//		exit(1);
//	}
//
//	inframe = av_frame_alloc();
//	if (!inframe) {
//		fprintf(log_file, "Could not allocate video frame\n");
//		exit(1);
//	}
//	inframe->format = AV_PIX_FMT_RGB32;
//	inframe->width = width;
//	inframe->height = height;
//
//	ret = av_frame_get_buffer(inframe, 32);
//	if (ret < 0) {
//		fprintf(log_file, "Could not allocate the video frame data\n");
//		exit(1);
//	}
//
//	/* initialize the decoder */
//	decoder = avcodec_find_decoder_by_name("h264_qsv");
//	if (!decoder) {
//		fprintf(log_file, "The QSV decoder is not present in libavcodec\n");
//		exit(1);
//	}
//	
//	decoder_ctx = avcodec_alloc_context3(decoder);
//	if (!decoder_ctx) {
//		ret = AVERROR(ENOMEM);
//		exit(1);
//	}
//	decoder_ctx->codec_id = AV_CODEC_ID_H264; 
//	decoder_ctx->refcounted_frames = 1;
//	decoder_ctx->opaque = &decodeContext;
//	decoder_ctx->get_format = get_format;
//
//
//	/* open the hardware device */
//	ret = av_hwdevice_ctx_create(&decodeContext.hw_device_ref, AV_HWDEVICE_TYPE_QSV,
//		"auto", NULL, 0);
//	if (ret < 0) {
//		fprintf(log_file, "Cannot open the hardware device\n");
//		char buf[1024];
//		av_strerror(ret, buf, sizeof(buf));
//		fprintf(log_file, "%s\n", buf);
//		exit(1);
//	}
//
//
//	/* open it */
//	ret = avcodec_open2(decoder_ctx, NULL, NULL);
//	if (ret < 0) {
//		fprintf(log_file, "Error opening the output context: \n");
//		exit(1);
//	}
//
//	frame = av_frame_alloc();
//	sw_frame = av_frame_alloc();
//	if (!frame || !sw_frame) {
//		ret = AVERROR(ENOMEM);
//		fprintf(log_file, "Error allocating the frames: \n");
//		exit(1);
//	}
//
//
//	//Capture flow is here
//	UCHAR* CaptureBuffer = NULL;
//	long CaptureLineSize = NULL;
//	ScreenCaptureProcessorGDI *D3D11 = new ScreenCaptureProcessorGDI();
//
//	//Capture part ends here.
//
//	D3D11->init();
//
//
//	CaptureBuffer = (UCHAR*)malloc(9000000);
//
//	rgb_to_nv12_SwsContext = sws_getContext(inframe->width, inframe->height, AV_PIX_FMT_RGB32,
//		outframe->width, outframe->height, AV_PIX_FMT_NV12,
//		SWS_POINT, NULL, NULL, NULL);
//	/* create scaling context */
//
//	if (!rgb_to_nv12_SwsContext) {
//		fprintf(log_file,
//			"Impossible to create scale context for the conversion "
//			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
//			av_get_pix_fmt_name(AV_PIX_FMT_RGB32), inframe->width, inframe->height,
//			av_get_pix_fmt_name(AV_PIX_FMT_NV12), outframe->width, outframe->height);
//		ret = AVERROR(EINVAL);
//
//	}
//
//	nv12_to_rgb_SwsContext = sws_getContext(iframe->width, iframe->height, AV_PIX_FMT_NV12,
//		oframe->width, oframe->height, AV_PIX_FMT_RGB32,
//		SWS_BICUBIC, NULL, NULL, NULL);
//	/* create scaling context */
//
//	if (!nv12_to_rgb_SwsContext) {
//		fprintf(log_file,
//			"Impossible to create scale context for the conversion "
//			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
//			av_get_pix_fmt_name(AV_PIX_FMT_NV12), iframe->width, iframe->height,
//			av_get_pix_fmt_name(AV_PIX_FMT_RGB32), oframe->width, oframe->height);
//		ret = AVERROR(EINVAL);
//
//	}
//
//	char buf[1024];
//
//	int number_of_frames_to_process = 100;
//	is_keyframe = (char *)malloc(number_of_frames_to_process + 10);
//
//	for (i = 0; i < number_of_frames_to_process; i++) 
//	{
//		fflush(log_file);
//		//Sleep(500);
//		/* make sure the frame data is writable */
//		/*ret = av_frame_make_writable(inframe);
//		if (ret < 0)
//			exit(1);*/
//
//		char * srcData = NULL;
//		//SleepEx(5, true);
//
//	
//		start = clock();
//		D3D11->GrabImage(CaptureBuffer, CaptureLineSize);
//		stop = clock();
//
//
//		nDiff = stop - start;
//		time_for_capture += nDiff;
//		number_of_frames_captured++;
//
//		//fprintf(file, "time for capture - %d\n", nDiff);
//
//		uint8_t * inData[1] = { (uint8_t *)CaptureBuffer }; // RGB have one plane
//		int inLinesize[1] = { av_image_get_linesize(AV_PIX_FMT_RGB32, inframe->width, 0) }; // RGB stride
//		
//		start = clock();
//		
//		int scale_rc = sws_scale(rgb_to_nv12_SwsContext, inData, inLinesize, 0, inframe->height, outframe->data,
//			outframe->linesize);
//
//		stop = clock();
//		nDiff = stop - start;
//		//fprintf(file, "time for scale - %d\n", nDiff);
//		time_for_scaling_rgb_to_yuv += nDiff;
//		number_of_rgb_frames_scaled++;
//
//		//char * filename24 = "captured";
//		//snprintf(buf, sizeof(buf), "E:\\h264-24\\%s-%d.bmp", filename24, i);
//
//		//pgm_save(inData[0], inframe->width, inframe->height, buf);
//
//		//outframe->data[0] = (uint8_t *)pgm_read24( outframe->width, outframe->height, buf);
//
//		outframe->pts = i;
//
//
//
//		/* encode the image */
//		start = clock();
//		encode(outframe, pkt);
//		stop = clock();
//		nDiff = stop - start;
//		time_for_encode += nDiff;
//		number_of_frames_encoded++;
//		//fprintf(file, "time for encode - %d\n", nDiff);
//
//		/* decode the image */
//		/*if (pkt->size != 0)
//		{
//			start = clock();
//			decode(pkt, "decoded");
//			stop = clock();
//			nDiff = stop - start;
//			tdec += nDiff;
//			ndec++;
//			fprintf(file, "time for decode - %d\n", nDiff);
//			av_packet_unref(pkt);
//		}*/
//
//	}
//
//	/* flush the encoder */
//	start = clock();
//	encode(NULL, pkt);
//	stop = clock();
//	nDiff = stop - start;
//	time_for_encode += nDiff;
//	number_of_frames_encoded++;
//
//	/* flush the decoder */
//	pkt->data = NULL;
//	pkt->size = 0;
//	//ret = decode_packet(&decodeContext, decoder_ctx, frame, sw_frame, pkt);
//
//	/* add sequence end code to have a real MPEG file */
//	//fwrite(endcode, 1, sizeof(endcode), h264_file);
//	fclose(h264_file);
//
//	//fwrite(endcode, 1, sizeof(endcode), outfile);
//	//fclose(outfile);
//
//	fprintf(log_file, "average time for scale - %f\n", time_for_scaling_rgb_to_yuv/ (float) number_of_rgb_frames_scaled);
//	fprintf(log_file, "average time for encode - %f\n", time_for_encode/ (float) number_of_frames_encoded);
//	/*fprintf(log_file, "average time for decode - %f\n", time_for_decode/ (float) number_of_packets_decoded);
//	fprintf(log_file, "average time for capture - %f\n", time_for_capture/ (float) number_of_frames_captured);
//	fprintf(log_file, "average size after encoding - %f\n", total_size_of_packets / (float) number_of_packets);
//	is_keyframe[decoder_ctx->frame_number] = '\0';
//	fwrite(is_keyframe, 1, decoder_ctx->frame_number+1, log_file);
//	fprintf(log_file, "\n", NULL);
//	fprintf(log_file, "Number of I - frames : %d\n", number_of_I_frames);
//	fprintf(log_file, "Number of P - frames : %d\n", number_of_packets);*/
//	free(is_keyframe);
//
//	fclose(log_file);
//
//	avcodec_close(encoder_ctx);
//	avcodec_close(decoder_ctx);
//	avcodec_free_context(&decoder_ctx);
//	av_frame_free(&sw_frame);
//	av_frame_free(&frame);
//	avcodec_free_context(&encoder_ctx);
//	av_frame_free(&inframe);
//	av_frame_free(&outframe);
//	av_packet_free(&pkt);
//
//	
//}

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



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
	{
		
		break;
	}
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_MOVE:

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if (!captured)
		{
			captured = true;
			driver(hWnd);
		}
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
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