/**
 * @file screen_to_file.cpp
 * @author Maximilian Otto (maxotto45@gmail.com)
 * @brief this code is meant to save the acquired frame to a file or clipboard and by code-projects.org 
 *        one can add this part to the end of the get_frame() function of the main.cpp to store a picture before returning
 * @version 1.0
 * @date 2021-10-15
 * 
 * @copyright Copyright (c) 2021 by Evgenyn Pereguda? https://www.codeproject.com/Tips/1116253/Desktop-Screen-Capture-on-Windows-via-Windows-Desk  
 */


#if 0  //#####################################################################################
	//save resource to file to see, if the screen got REALLY captured
	//SO.: code-project.org
	BITMAPINFO lBmpInfo;

	// BMP 32 bpp
	ZeroMemory(&lBmpInfo, sizeof(BITMAPINFO));
	lBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lBmpInfo.bmiHeader.biBitCount = 32;
	lBmpInfo.bmiHeader.biCompression = BI_RGB;
	lBmpInfo.bmiHeader.biWidth = desktop_duplicate_desc.ModeDesc.Width;
	lBmpInfo.bmiHeader.biHeight = desktop_duplicate_desc.ModeDesc.Height;
	lBmpInfo.bmiHeader.biPlanes = 1;
	lBmpInfo.bmiHeader.biSizeImage = desktop_duplicate_desc.ModeDesc.Width
		* desktop_duplicate_desc.ModeDesc.Height * 4;

	std::unique_ptr<BYTE> pBuf(new BYTE[lBmpInfo.bmiHeader.biSizeImage]);
	UINT lBmpRowPitch = desktop_duplicate_desc.ModeDesc.Width * 4;

	BYTE* sptr = reinterpret_cast<BYTE*>(mapped_subresource.pData);
	BYTE* dptr = pBuf.get() + lBmpInfo.bmiHeader.biSizeImage - lBmpRowPitch;

	UINT lRowPitch = std::min<UINT>(lBmpRowPitch, mapped_subresource.RowPitch);

	for (size_t h = 0; h < desktop_duplicate_desc.ModeDesc.Height; ++h) {
		memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
		sptr += mapped_subresource.RowPitch;
		dptr -= lBmpRowPitch;
	}

	// Save bitmap buffer into the file ScreenShot.bmp
	WCHAR lMyDocPath[MAX_PATH];
	hr = SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, lMyDocPath);
	if (FAILED(hr))
		std::cout << "Fail bitmap 1\n";

	std::wstring lFilePath = std::wstring(lMyDocPath) + L"\\ScreenShot.bmp";

	FILE* lfile = nullptr;

	auto lerr = _wfopen_s(&lfile, lFilePath.c_str(), L"wb");
	if (lerr != 0)
		std::cout << "Fail bitmap 2\n";

	if (lfile != nullptr) {

		BITMAPFILEHEADER	bmpFileHeader;
		bmpFileHeader.bfReserved1 = 0;
		bmpFileHeader.bfReserved2 = 0;
		bmpFileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + lBmpInfo.bmiHeader.biSizeImage;
		bmpFileHeader.bfType = 'MB';
		bmpFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		fwrite(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, lfile);
		fwrite(&lBmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), 1, lfile);
		fwrite(pBuf.get(), lBmpInfo.bmiHeader.biSizeImage, 1, lfile);

		fclose(lfile);
		ShellExecute(0, 0, lFilePath.c_str(), 0, 0, SW_SHOW);
	}
#endif //#####################################################################################
