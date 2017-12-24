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
	bi.biHeight = height;
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

void decode(AVPacket *pkt)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(decodingCodecContext, pkt);
	if (ret < 0) 
	{
		fprintf(log_file, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decodingCodecContext, iframe);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(log_file, "Error during decoding\n");
			exit(1);
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

		// Note : row_pitch / linesize = width * 4 + padding

		FILE *f;
		fopen_s(&f, buf, "wb");
		DWORD dwBytesWritten = 0;
		dwBytesWritten = sizeof(AVFrame);
		dwBytesWritten += fwrite(iframe, sizeof(AVFrame), 1, f);
		dwBytesWritten += fwrite(iframe->data[0], iframe->linesize[0] * iframe->height, 1, f);
		dwBytesWritten += fwrite(iframe->data[1], iframe->linesize[1] * iframe->height/2, 1, f);
		dwBytesWritten += fwrite(iframe->data[2], iframe->linesize[2] * iframe->height/2, 1, f);
		fclose(f);

		/*int dist = 0;
		for (int h = 0; h < oframe->height; )
		{
			for (int w = 0; w < oframe->width; w++)
			{
				dist = iframe->linesize[0] * h;
				oframe->data[0][ oframe->linesize[0] * h + w * 4] = iframe->data[2][dist + w];
				oframe->data[0][oframe->linesize[0] * h + w * 4 + 1] = iframe->data[1][dist + w];
				oframe->data[0][oframe->linesize[0] * h + w * 4 + 2] = iframe->data[0][dist + w];
				oframe->data[0][oframe->linesize[0] * h + w * 4 + 3] = 255;
			}
			h = h + 1;
		}*/

		int scale_rc = sws_scale(yuv_to_rgb_SwsContext
								, iframe->data, iframe->linesize
								, 0, iframe->height
								, oframe->data, oframe->linesize);

		//fwrite(oframe->data[0], 1, oframe->width*3*oframe->height, outfile);
		_snprintf_s(buf, sizeof(buf), "%s\\Decoded\\_%d.bmp", pwd, decodingCodecContext->frame_number);
		save_as_bitmap(oframe->data[0], oframe->linesize[0], oframe->width, oframe->height, buf);
	}
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
		exit(1);
	}
	
	while (ret >= 0) 
	{
		ret = avcodec_receive_packet(encodingCodecContext, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}
			
		else if (ret < 0) {
			fprintf(log_file, "Error during encoding\n");
			exit(1);
		}

		//printf("Write packet %d (size=%5d)\n", pkt->pts, pkt->size);
		
		start_decode = clock();
		decode(pkt);
		stop_decode = clock();
		nDiff_decode = stop_decode - start_decode;
		time_for_decode += nDiff_decode;
		number_of_packets_decoded++;
		//fprintf(log_file, "time for decode - %d\n", nDiff);

		//fprintf(log_file, "size of packet- %d\n", pkt->size);
		total_size_of_packets += pkt->size;
		number_of_packets++;
		fwrite(pkt->data, 1, pkt->size, h264_file);
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

void driver(HWND hWnd)
{
	const char *filename, *codec_name, *folder_name;
	const AVCodec *codec, *decodec;
	int i, ret, x, y;
	AVPacket *pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	fopen_s(&log_file, "log.txt_new", "w");

	GetCurrentDirectory(MAX_PATH, pwd);


	folder_name = "Captured";

	//createDirectory(folder_name);

	folder_name = "Decoded";

	//createDirectory(folder_name);
	
	filename = "encoded.h264";
	//const char *outfilename = "decoded.avi";
	codec_name = "libx264";
	//MessageBox(hWnd, (LPCWSTR)"1", (LPCWSTR)"1", 1);
	avcodec_register_all();

	/* find the video encoder */
	codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec) {
		fprintf(log_file, "Codec '%s' not found\n", codec_name);
		exit(1);
	}	

	encodingCodecContext = avcodec_alloc_context3(codec);
	if (!encodingCodecContext) {
		fprintf(log_file, "Could not allocate video codec context\n");
		exit(1);
	}
	
	pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	/* put sample parameters */
	encodingCodecContext->bit_rate = 4000000;
	/* resolution must be a multiple of two */
	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(NULL, &rcClient);

	int width = 1366, height = 768;

	RECT desktop;
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


	encodingCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	

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
		exit(1);
	}	


	fopen_s(&h264_file, filename, "wb");
	if (!h264_file) {
		fprintf(log_file, "Could not open %s\n", filename);
		exit(1);
	}

	/*fopen_s(&outfile, outfilename, "wb");
	if (!outfile) {
		fprintf(log_file, "Could not open %s\n", outfilename);
		exit(1);
	}*/

	outframe = av_frame_alloc();
	if (!outframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		exit(1);
	}
	outframe->format = AV_PIX_FMT_YUV420P;
	outframe->width = encodingCodecContext->width;
	outframe->height = encodingCodecContext->height;



	ret = av_frame_get_buffer(outframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		exit(1);
	}

	inframe = av_frame_alloc();
	if (!inframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		exit(1);
	}
	inframe->format = AV_PIX_FMT_RGB32;
	inframe->width = width;
	inframe->height = height;

	ret = av_frame_get_buffer(inframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		exit(1);
	}

	iframe = av_frame_alloc();
	if (!iframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		exit(1);
	}

	iframe->format = AV_PIX_FMT_YUV420P;
	iframe->width = encodingCodecContext->width;
	iframe->height = encodingCodecContext->height;

	ret = av_frame_get_buffer(iframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		exit(1);
	}

	oframe = av_frame_alloc();
	if (!oframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		exit(1);
	}

	oframe->format = AV_PIX_FMT_RGB32;
	oframe->width = (width + 7) / 8 * 8;
	oframe->height = height;

	ret = av_frame_get_buffer(oframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		exit(1);
	}


	//decoder
	/* find the video decoder */
	decodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!decodec) {
		fprintf(log_file, "Codec '%s' not found\n", codec_name);
		exit(1);
	}

	decodingCodecContext = avcodec_alloc_context3(decodec);
	if (!decodingCodecContext) {
		fprintf(log_file, "Could not allocate video codec context\n");
		exit(1);
	}

	decodingCodecContext->width = width;
	decodingCodecContext->height = height;
	//dc->pix_fmt = AV_PIX_FMT_YUV420P;// AV_PIX_FMT_RGB32;

	/* open it */
	ret = avcodec_open2(decodingCodecContext, decodec, NULL);
	if (ret < 0) {
		printf("Could not open codec: \n");
		exit(1);
	}


	//Capture flow is here
	UCHAR* CaptureBuffer = NULL;
	long CaptureLineSize = NULL;
	ScreenCaptureProcessorGDI *D3D11 = new ScreenCaptureProcessorGDI();

	//Capture part ends here.

	D3D11->init();


	CaptureBuffer = (UCHAR*)malloc(9000000000);

	rgb_to_yuv_SwsContext = sws_getContext(inframe->width, inframe->height, AV_PIX_FMT_RGB32,
		outframe->width, outframe->height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);
	/* create scaling context */

	if (!rgb_to_yuv_SwsContext) {
		fprintf(log_file,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name(AV_PIX_FMT_RGB32), inframe->width, inframe->height,
			av_get_pix_fmt_name(AV_PIX_FMT_YUV420P), outframe->width, outframe->height);
		ret = AVERROR(EINVAL);

	}

	/* create scaling context */
	yuv_to_rgb_SwsContext = sws_getContext(iframe->width, iframe->height, AV_PIX_FMT_YUV420P,
		oframe->width, oframe->height, AV_PIX_FMT_RGB32,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!yuv_to_rgb_SwsContext) {
		fprintf(log_file,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name(AV_PIX_FMT_YUV420P), iframe->width, iframe->height, av_get_pix_fmt_name(AV_PIX_FMT_RGB32), oframe->width, oframe->height);
		ret = AVERROR(EINVAL);
		//goto end;
	}

	int *invt = (int*)malloc(sizeof(int) * 4);
	int *tabl = (int*)malloc(sizeof(int) * 4);

	int ** inv_table = &invt;
	int *srcRange = (int*)malloc(sizeof(int));
	int **table = &tabl;
	int *dstRange = (int*)malloc(sizeof(int));
	int *brightness = (int*)malloc(sizeof(int));
	int *contrast = (int*)malloc(sizeof(int));
	int *saturation = (int*)malloc(sizeof(int));
	int cs = sws_getColorspaceDetails(yuv_to_rgb_SwsContext, inv_table, srcRange, table, dstRange, brightness, contrast, saturation);
	const int * coe = sws_getCoefficients(cs);
	cs = SWS_CS_DEFAULT;
	fprintf(log_file, "%d %d %d\n%d %d %d\n%d %d %d\n", (uint8_t)(coe[0]), (uint8_t)(coe[1]), (uint8_t)coe[2], (uint8_t)coe[3], (uint8_t)coe[4], (uint8_t)coe[5], (uint8_t)coe[6], (uint8_t)coe[7], (uint8_t)coe[8]);
	fprintf(log_file, "%d %d %d %d\n", (uint8_t)invt[0], (uint8_t)invt[1], (uint8_t)invt[2], (uint8_t)invt[3]);
	fprintf(log_file, "%d %d %d %d\n", (uint8_t)tabl[0], (uint8_t)tabl[1], (uint8_t)tabl[2], (uint8_t)tabl[3]);
	fprintf(log_file, "%d %d\n", srcRange[0], dstRange[0]);
	fprintf(log_file, "%d %d %d\n", brightness[0], contrast[0], saturation[0]);

	cs = sws_getColorspaceDetails(rgb_to_yuv_SwsContext, inv_table, srcRange, table, dstRange, brightness, contrast, saturation);
	const int *coe1 = sws_getCoefficients(cs);

	fprintf(log_file, "SWS_CS_BT2020\n%d %d %d\n%d %d %d\n%d %d %d\n", (uint8_t)coe1[0], (uint8_t)coe1[1], (uint8_t)coe1[2], (uint8_t)coe1[3], (uint8_t)coe1[4], (uint8_t)coe1[5], (uint8_t)coe1[6], (uint8_t)coe1[7], (uint8_t)coe1[8]);
	

	/*free(invt);
	free(tabl);
	free(srcRange);
	free(dstRange);
	free(brightness);
	free(contrast);
	free(saturation);*/


	char buf[1024];

	int number_of_frames_to_process = 2;
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
		int inLinesize[1] = { CaptureLineSize/inframe->height /*av_image_get_linesize(AV_PIX_FMT_RGB32, inframe->width, 0)*//*inframe->width * 4*/ }; // RGB stride
		
		start = clock();
		
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

	/* add sequence end code to have a real MPEG file */
	fwrite(endcode, 1, sizeof(endcode), h264_file);
	fclose(h264_file);

	//fwrite(endcode, 1, sizeof(endcode), outfile);
	//fclose(outfile);

	fprintf(log_file, "average time for scale - %f\n", time_for_scaling_rgb_to_yuv/ (float) number_of_rgb_frames_scaled);
	fprintf(log_file, "average time for encode - %f\n", time_for_encode/ (float) number_of_frames_encoded);
	//fprintf(log_file, "average time for decode - %f\n", time_for_decode/(float) number_of_packets_decoded);
	fprintf(log_file, "average time for capture - %f\n", time_for_capture/ (float) number_of_frames_captured);
	fprintf(log_file, "average size after encoding - %f\n", total_size_of_packets / (float)number_of_packets);
	is_keyframe[decodingCodecContext->frame_number] = '\0';
	fwrite(is_keyframe, 1, decodingCodecContext->frame_number+1, log_file);
	fprintf(log_file, "\n", NULL);
	fprintf(log_file, "Number of I - frames : %d\n", number_of_I_frames);
	fprintf(log_file, "Number of P - frames : %d\n", number_of_packets);
	free(is_keyframe);

	fclose(log_file);

	avcodec_close(encodingCodecContext);
	avcodec_close(decodingCodecContext);
	avcodec_free_context(&decodingCodecContext);
	av_frame_free(&iframe);
	av_frame_free(&oframe);
	avcodec_free_context(&encodingCodecContext);
	av_frame_free(&inframe);
	av_frame_free(&outframe);
	av_packet_free(&pkt);

	
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