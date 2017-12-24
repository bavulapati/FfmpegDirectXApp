#include "stdafx.h"
#include "RPCScreenCapture.h"
#include "time.h"

D3D_DRIVER_TYPE gDriverTypes[] =
{
	D3D_DRIVER_TYPE_HARDWARE
};
UINT gNumDriverTypes = ARRAYSIZE(gDriverTypes);

// Feature levels supported
D3D_FEATURE_LEVEL gFeatureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_1
};

UINT gNumFeatureLevels = ARRAYSIZE(gFeatureLevels);

ScreenCaptureProcessorGDI::ScreenCaptureProcessorGDI() 
	{
		
	}


RPCScreenCapture::RPCScreenCapture(void)
{
}

RPCScreenCapture::~RPCScreenCapture(void)
{
}

bool ScreenCaptureProcessorGDI::init()
	{
		// Create device			
		for (UINT DriverTypeIndex = 0; DriverTypeIndex < gNumDriverTypes; ++DriverTypeIndex)
		{
			hr = D3D11CreateDevice(
				nullptr,
				gDriverTypes[DriverTypeIndex],
				nullptr,
				0,
				gFeatureLevels,
				gNumFeatureLevels,
				D3D11_SDK_VERSION,
				&lDevice,
				&lFeatureLevel,
				&lImmediateContext);

			if (SUCCEEDED(hr))
			{
				// Device creation success, no need to loop anymore
				break;
			}

			lDevice.Release();

			lImmediateContext.Release();
		}

		if (FAILED(hr))
		{
			return false;
		}

		if (lDevice == nullptr)
		{
			return false;
		}

		// Get DXGI device
		CComPtrCustom<IDXGIDevice> lDxgiDevice;

		hr = lDevice->QueryInterface(IID_PPV_ARGS(&lDxgiDevice));

		if (FAILED(hr))
		{
			return false;
		}

		// Get DXGI adapter
		CComPtrCustom<IDXGIAdapter> lDxgiAdapter;
		hr = lDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&lDxgiAdapter));

		if (FAILED(hr))
		{
			return false;
		}

		lDxgiDevice.Release();

		UINT Output = 0;

		// Get output
		CComPtrCustom<IDXGIOutput> lDxgiOutput;
		hr = lDxgiAdapter->EnumOutputs(
			Output,
			&lDxgiOutput);

		if (FAILED(hr))
		{
			return false;
		}

		lDxgiAdapter.Release();

		hr = lDxgiOutput->GetDesc(
			&lOutputDesc);

		if (FAILED(hr))
		{
			return false;
		}

		// QI for Output 1
		CComPtrCustom<IDXGIOutput1> lDxgiOutput1;

		hr = lDxgiOutput->QueryInterface(IID_PPV_ARGS(&lDxgiOutput1));

		if (FAILED(hr))
		{
			return false;
		}

		lDxgiOutput.Release();

		// Create desktop duplication
		hr = lDxgiOutput1->DuplicateOutput(
			lDevice,
			&lDeskDupl);

		if (FAILED(hr))
		{
			return false;
		}

		lDxgiOutput1.Release();

		// Create GUI drawing texture
		lDeskDupl->GetDesc(&lOutputDuplDesc);

		D3D11_TEXTURE2D_DESC desc;

		desc.Width = lOutputDuplDesc.ModeDesc.Width;

		desc.Height = lOutputDuplDesc.ModeDesc.Height;

		desc.Format = lOutputDuplDesc.ModeDesc.Format;

		desc.ArraySize = 1;

		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;

		desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

		desc.SampleDesc.Count = 1;

		desc.SampleDesc.Quality = 0;

		desc.MipLevels = 1;

		desc.CPUAccessFlags = 0;

		desc.Usage = D3D11_USAGE_DEFAULT;

		hr = lDevice->CreateTexture2D(&desc, NULL, &lGDIImage);

		if (FAILED(hr))
		{
			return false;
		}

		if (lGDIImage == nullptr)
		{
			return false;
		}

		// Create CPU access texture

		desc.Width = lOutputDuplDesc.ModeDesc.Width;

		desc.Height = lOutputDuplDesc.ModeDesc.Height;

		desc.Format = lOutputDuplDesc.ModeDesc.Format;

		desc.ArraySize = 1;

		desc.BindFlags = 0;

		desc.MiscFlags = 0;

		desc.SampleDesc.Count = 1;

		desc.SampleDesc.Quality = 0;

		desc.MipLevels = 1;

		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_STAGING;

		hr = lDevice->CreateTexture2D(&desc, NULL, &lDestImage);

		if (FAILED(hr))
		{
			return false;
		}

		if (lDestImage == nullptr)
		{
			return false;
		}

		return true;
	}


	bool ScreenCaptureProcessorGDI::GrabImage(UCHAR* &CaptureBuffer, long &CaptureLineSize)
	{
		int lTryCount = 4;
		CComPtrCustom<IDXGIResource> lDesktopResource;
		CComPtrCustom<ID3D11Texture2D> lAcquiredDesktopImage;
		DXGI_OUTDUPL_FRAME_INFO lFrameInfo;

		do
		{
			hr = lDeskDupl->AcquireNextFrame(INFINITE, &lFrameInfo, &lDesktopResource);

			if (SUCCEEDED(hr))
				break;

			if (hr == DXGI_ERROR_WAIT_TIMEOUT)
			{
				continue;
			}
			else if (FAILED(hr))
			{
				break;
			}

		} while (--lTryCount > 0);

		if (FAILED(hr))
		{
			error = true;
			return false;
		}

		hr = lDesktopResource->QueryInterface(IID_PPV_ARGS(&lAcquiredDesktopImage));

		lDesktopResource.Release();

		if (FAILED(hr))
		{
			error = true;
			return false;
		}

		if (lAcquiredDesktopImage == nullptr)
		{
			error = true;
			return false;
		}

		// Copy image into GDI drawing texture
		lImmediateContext->CopyResource(lGDIImage, lAcquiredDesktopImage);

		lAcquiredDesktopImage.Release();

		lDeskDupl->ReleaseFrame();

		// Copy image into CPU access texture
		lImmediateContext->CopyResource(lDestImage, lGDIImage);

		// Copy from CPU access texture to bitmap buffer
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		lImmediateContext->Map(lDestImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource);

		BITMAPINFO	lBmpInfo;

		// BMP 32 bpp
		ZeroMemory(&lBmpInfo, sizeof(BITMAPINFO));

		lBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

		lBmpInfo.bmiHeader.biBitCount = 32;

		lBmpInfo.bmiHeader.biCompression = BI_RGB;

		lBmpInfo.bmiHeader.biWidth = lOutputDuplDesc.ModeDesc.Width;

		lBmpInfo.bmiHeader.biHeight = lOutputDuplDesc.ModeDesc.Height;

		lBmpInfo.bmiHeader.biPlanes = 1;

		lBmpInfo.bmiHeader.biSizeImage = lOutputDuplDesc.ModeDesc.Width
			* lOutputDuplDesc.ModeDesc.Height * 4;

		//std::unique_ptr<BYTE> pBuf(CaptureBuffer);

		UINT lBmpRowPitch = lOutputDuplDesc.ModeDesc.Width * 4;

		BYTE* sptr = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* dptr = CaptureBuffer /*+ lBmpInfo.bmiHeader.biSizeImage - lBmpRowPitch*/;

		UINT lRowPitch = std::min<UINT>(lBmpRowPitch, resource.RowPitch);

		
		clock_t start = clock();
		for (size_t h = 0; h < lOutputDuplDesc.ModeDesc.Height; ++h)
		{
			memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
			sptr += resource.RowPitch;
			dptr += lBmpRowPitch;
		}
		clock_t stop = clock();

		clock_t nDiff = stop - start;

		//fprintf(log_file, "average time for scale - %ld\n", nDiff);


		lImmediateContext->Unmap(lDestImage, subresource);

		CaptureLineSize = lBmpRowPitch/*lBmpInfo.bmiHeader.biSizeImage*/;
		//CaptureBuffer= new UCHAR[CaptureSize];
		//CaptureBuffer = (UCHAR*)malloc(CaptureSize);
		//memcpy(CaptureBuffer, pBuf.get(), lBmpInfo.bmiHeader.biSizeImage);
		
		//saveImage(frame_count, lBmpInfo, std::move(pBuf));

		frame_count++;

		return true;
	}

#pragma region Save Image 

	void ScreenCaptureProcessorGDI::saveImage(unsigned int frame_num, BITMAPINFO &lBmpInfo, std::unique_ptr<BYTE> pBuf)
	{
		std::wstring lMyDocPath = L"C:\\Bala\\captured";
		std::wstring lFilePath = std::wstring(lMyDocPath) + L"\\ScreenShot" + std::to_wstring(frame_num) + L".bmp"; //std::wstring(lMyDocPath) + L"\\ScreenShot.bmp";


		FILE* lfile = nullptr;
		auto lerr = _wfopen_s(&lfile, lFilePath.c_str(), L"wb");

		if (lerr != 0) 
		{			
			return;
		}

		if (lfile != nullptr)
		{
			BITMAPFILEHEADER bmpFileHeader;

			bmpFileHeader.bfReserved1 = 0;
			bmpFileHeader.bfReserved2 = 0;
			bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + lBmpInfo.bmiHeader.biSizeImage;
			bmpFileHeader.bfType = 'MB';
			bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

			/*CaptureSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + lBmpInfo.bmiHeader.biSizeImage;
			CaptureBuffer = (UCHAR*)malloc(CaptureSize);*/

			fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, lfile);
			fwrite(&lBmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, lfile);
			fwrite(pBuf.get(), lBmpInfo.bmiHeader.biSizeImage, 1, lfile);

			fclose(lfile);

		}
	}

#pragma endregion
	bool ScreenCaptureProcessorGDI::release()
	{
		return true;
	}

	void ScreenCaptureProcessorGDI::setMaxFrames(unsigned int maxFrames)
	{
		max_count_frames = maxFrames;
	}

	bool ScreenCaptureProcessorGDI::hasFailed()
	{
		return error == true;
	}





	
