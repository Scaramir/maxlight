#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <sstream> 
#include <stdlib.h>
//#include <DXGI.h>
#include <dxgi1_2.h>        //include-order
//#include <d3d12.h>        //let's make d11 first
#include <d3d11.h>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <conio.h>
#include <chrono>
#include <shlobj.h>
#include <shellapi.h>

#include "ccomptrcustom_class.hpp"
#include "SerialClass.h"

#pragma comment(lib, "dxgi") //this is for CreateDXGIFactory1()
#pragma comment(lib, "d3d11")
//#pragma comment(lib, "windowscodecs.lib")


//#############################################################################################
// Driver types supported
D3D_DRIVER_TYPE gDriverTypes[] = {D3D_DRIVER_TYPE_HARDWARE};
UINT gNumDriverTypes = ARRAYSIZE(gDriverTypes);

// Customized feature level support
D3D_FEATURE_LEVEL gFeatureLevels[] = {
	D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_1,
};
UINT gNumFeatureLevels = ARRAYSIZE(gFeatureLevels);

//#############################################################################################
///Declaration of global variables:
namespace screen_capture {
//main:
	UINT sleepTimerMs = (int)(((float)1 / 24) * 1000);
	UINT fps = 30;
	void set_sleepTimerMs(unsigned int& fps) { if (fps == 0) sleepTimerMs = 0; else sleepTimerMs = ((int)(((float)1 / fps) * 1000)-1); }
	HRESULT hr = E_FAIL;
//output_enumeration() + check_monitor_devices()
	std::vector<IDXGIAdapter1*> adapters;							//Needs to be Released()
	int chosen_adapter_num = 0;										//default adpater 
	IDXGIAdapter1* chosen_adapter = nullptr;
	int monitor_num = 0;											//outputs index
	std::vector<IDXGIOutput*> outputs;
	CComPtrCustom<IDXGIOutput> output = nullptr;
	int chosen_output_num = 0;
	IDXGIOutput* chosen_output = nullptr;
//check_cpu_access()
	D3D11_TEXTURE2D_DESC texture_desc;
	D3D11_SUBRESOURCE_DATA sub_data;
	D3D11_MAPPED_SUBRESOURCE map;
//create_and_get_device()
	D3D_FEATURE_LEVEL feature_level;
	CComPtrCustom<ID3D11Device> device = nullptr;
	CComPtrCustom<ID3D11DeviceContext> context = nullptr;
	CComPtrCustom<IDXGIOutput1> output1 = nullptr;
	CComPtrCustom<IDXGIOutputDuplication> desktop_duplication = nullptr;
//reject_sub_pixel()
	UINT8 min_saturation_per_pixel = 20; //60;
	UINT8 min_brightness_per_pixel = 100; //160;
//get_frame()
	bool new_frame = false;
	DXGI_OUTDUPL_DESC desktop_duplicate_desc;
	CComPtrCustom<IDXGIResource> frame = nullptr;
	DXGI_OUTDUPL_FRAME_INFO frame_info;
	CComPtrCustom<ID3D11Texture2D> frame_texture = nullptr;
	IDXGIResource* pDesktopResource = nullptr;
	CComPtrCustom<ID3D11Texture2D> frame_texture_ori = nullptr;
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
//benchmark
	int acquired_frames_count = 0;
	int mapped_frames_counter = 0;
	int fail_invalid = 0;
	int fail_1 = 0;
	int fail_2 = 0;
	int fail_3 = 0;
	int fail_4 = 0;
	int fail_5 = 0;
//arduino connection 
	const char serial_port[] = { 'C', 'O', 'M', '7' };	//usb port name 
	Serial* SP;
//led_stuff:
	struct Pixel {
	public:
		int b = 0;	                   //initialized to 'black'; 
		int g = 0;
		int r = 0;
	};
	//retrieve_pixel():
		Pixel curr_pixel;
		Pixel accum_pixel;
		Pixel mean_pixel;
	//fade:
	int fade_val = 100;				//default value. Adafruits prefers 75
	Pixel mean_color_old;
	Pixel mean_color_new;


	const uint8_t gamma8[] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    
							   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,    
							   1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,    
							   2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
							   5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,  
							   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,   
							   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,   
							   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,   
							   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,   
							   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,   
							   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,   
							   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,  
							   115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,  
							   144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,  
							   177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,  
							   215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
}; using namespace screen_capture;

//#############################################################################################
///SO: https://github.com/diederickh/screen_capture/blob/master/src/test/test_win_api_directx_research.cpp;
//Check Devices for outputs
/**
 * @brief check monitors (outputs) for a graphic adpater and push it into vec<output>
 * @param i index of adapter
 * @return
 */
int output_enumeration(INT16& i) {
	int dx = 0;
	while (DXGI_ERROR_NOT_FOUND != adapters[i]->EnumOutputs(dx, &output)) {

		std::cout << "\t  (" << outputs.size() << ".) ";
		printf("Found monitor %d on adapter: %lu \n", monitor_num, i);

		outputs.push_back(output);	//store the found monitor

		DXGI_OUTPUT_DESC desc;		//get description of the monitor
		HRESULT hr = outputs[monitor_num]->GetDesc(&desc);							
		if (SUCCEEDED(hr)) {		//print info
			wprintf(L"\t\tMonitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'Y' : 'n');
			std::cout << "\t\twith the following dimensions:\n\t\t " <<
			abs(abs((int)desc.DesktopCoordinates.right) - abs((int)desc.DesktopCoordinates.left)) <<
				" x " <<
			abs(abs((int)desc.DesktopCoordinates.top) - abs((int)desc.DesktopCoordinates.bottom)) <<
				" pixel" << "\n"; 
		} else {
			printf("Error: failed to retrieve a DXGI_OUTPUT_DESC for output %lu.\nContinue\n", i);
			continue;
		}
		++monitor_num; ++dx;
	}
	return dx; //
}

//Check for devices/adapters, monitors, and choose one
/**
 * @brief check for adapters and devices; choose one
 * @return chosen_output_num
 */
int check_monitor_devices() {
	HRESULT hr = E_FAIL;

	//Create device that's able to enumerate @adapters
	IDXGIFactory1* factory = nullptr;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory)); //winSDK and x64 needed for compilation
	if (FAILED(hr)) { //hr != 0 || S_OK
		printf("Error: failed to retrieve the IDXGIFactory.\n");
		exit(EXIT_FAILURE);
		return -1;
	}

	//Enumerate the @adapters aka GPUs
	IDXGIAdapter1* adapter = nullptr;
	INT16 i = 0;
	while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter)) {
		adapters.push_back(adapter);
		++i;
	}
	if (adapters.empty()) {
		printf("Error: no adapter found. Enumaration fails accordingly. \n\tSomething went really wrong! ...\n");
		exit(EXIT_FAILURE);
		return -1;
	}

	//Print list of adapters and push corresponding @outputs
	std::cout << "Following graphics adapters (GPUs) are found:" << "\n";
	for (INT16 i = 0; i < adapters.size(); ++i) {
		DXGI_ADAPTER_DESC1 desc;
		hr = adapters[i]->GetDesc1(&desc);

		if (SUCCEEDED(hr)) {
			wprintf(L"Adapter: %lu, description: %s\n\t..searching for monitors..\n", i, desc.Description); //wide-character for UNICODE
			int found_on_adapter = output_enumeration(i);
			std::cout << "\t" << found_on_adapter << " monitor(s) found.\n\n";
		} else {
			printf("\tError: failed to get a description for the adapter: %lu\n  Please check your drivers.\n", i);
			continue;
		}
	}

	//select monitor and adapter for analyzation
		std::cout << "\nWhich monitor do you choose?\n\tType the number (x.) of the monitor." << "\n";
		std::cin.clear(); //yaya, use scanl, getline, anything but cin 
		std::cin >> chosen_output_num; 
		///fix device creation for this:
		//std::cout << "Which graphics adapter do you choose?\n\tType the number of adapter." << "\n";
		std::cin.clear();
		//std::cin >> chosen_adapter_num;

	return chosen_output_num;
}

/**
 * @brief creates a cpu access texture (D3D11Texture2D); this way, the texture can get copied and mapped.
 * this can be stored in system memory or as a shader-texture
 * @TODO: Check performance for memcpy and cpu_access
 */
int check_cpu_access_texture() {
//create a texture with cpu_access_read
	if (frame_texture != nullptr)
		return 0; 
	else 
		printf("Creating new 2DTexture\n");
	
	sub_data = {
		sub_data.pSysMem = std::calloc(texture_desc.Width * texture_desc.Height, 4),
		sub_data.SysMemPitch = 4 * texture_desc.Width,
		sub_data.SysMemSlicePitch = 0
	};

	HRESULT hr = device->CreateTexture2D(&texture_desc, &sub_data, &frame_texture);
	if (S_OK!=hr) {
		printf("Error: Failed to create the 2DTexture.\n");
		return -5;
	} else
		printf("'CreateTexture2D()' was successful!\n");

	return 0;
}

//create D3DX-Device and query interfaces 
/**
 * @brief create device (D3D11) for @chosen_monitor
 * @param chosen_monitor number of monitor (retrieve of @outputs)
 * @return success (0) or fail (negativ) 
 */
int create_and_get_device(int &chosen_monitor) {
	for (UINT driver_type_index = 0; driver_type_index < gNumDriverTypes; ++driver_type_index) {
		hr = D3D11CreateDevice(
				nullptr,						// Adapter: The adapter (video card) we want to use. We may use nullptr to pick the default adapter. 
				gDriverTypes[driver_type_index],// DriverType: We use the GPU as backing device. 
				nullptr,						// Software: we're using a D3D_DRIVER_TYPE_HARDWARE so it's not applicaple.
				0,								// D3D11_CREATE_DEVICE_FLAG 
				gFeatureLevels,					// Feature Levels (ptr to array): order of versions to use. 
				gNumFeatureLevels,				// Number of feature levels. according to defaults Array
				D3D11_SDK_VERSION,				// The SDK version, use D3D11_SDK_VERSION 
				&device,						// OUT: the ID3D11Device object. 
				&feature_level,					// OUT: the selected feature level. 
				&context						// OUT: the ID3D11DeviceContext that represents the above features. 
		     );						
		if (SUCCEEDED(hr)) {
			std::cout << "Device creation succesful." << "\n";
			break;
		}
	} 
	if (hr == E_INVALIDARG) {                   //in Case D3D11_1 does not work (Error: invalid_arguments passed), take D3D11 and standard notation/parameters
	    hr = D3D11CreateDevice(
			   nullptr,                         //outputAdapter
			   D3D_DRIVER_TYPE_HARDWARE, 
			   nullptr,
			   0,
			   &gFeatureLevels[1],
			   gNumFeatureLevels - 1,
			   D3D11_SDK_VERSION,
			   &device,                         //TODO: enter specific graphic device, adapter and monitor in this section
			   &feature_level,
			   &context
		    );
		if(SUCCEEDED(hr))
			std::cout << "Device creation succesful (feature level: D3D11)." << "\n";
	} 
	if (FAILED(hr) || device == nullptr) {
			printf("Error: failed to create a D3D11 Device and Context.\n"); 
			context.Release();
			exit(EXIT_FAILURE);
			return -1;
	}

	Sleep(100); 

	/**
	 * Create a IDXGIOutputDuplication
	 * query a IDXGIOutput1, because the IDXGIOutput1 has the DuplicateOutput feature.
	 */
	chosen_output = outputs[chosen_output_num];
	if (chosen_output == nullptr) {
		std::cout << "Selected Monitor '" << chosen_output_num << "' is invalid. ";
		exit(EXIT_FAILURE);
		return -2;
	}

	hr = chosen_output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
	if (FAILED(hr)) {
		std::cout << "QueryInterface failed." << "\n";
		exit(EXIT_FAILURE);
		return -3;
	}
	chosen_output->Release();

	hr = output1->DuplicateOutput(device, &desktop_duplication);
	if (FAILED(hr)) {
		std::cout << "Error: Desktop duplication failed." << "\n";
		exit(EXIT_FAILURE);
		return -4; 
	}
	output1.Release();

	std::cout << "Desktop duplication device created succesfully " << "\n";

	//texture description
	desktop_duplication->GetDesc(&desktop_duplicate_desc);
	ZeroMemory(&texture_desc, sizeof(texture_desc));
	texture_desc.Width = desktop_duplicate_desc.ModeDesc.Width;
	texture_desc.Height = desktop_duplicate_desc.ModeDesc.Height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                                                        // This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_STAGING /*D3D11_USAGE_DYNAMIC*/;
	texture_desc.BindFlags = 0 /*D3D11_BIND_SHADER_RESOURCE*/;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE /*D3D11_CPU_ACCESS_WRITE*/; // 0 doesn't work -> invalid_arg
	texture_desc.MiscFlags = 0;

	std::cout << "Capturing a " << texture_desc.Width << " x " << texture_desc.Height << " monitor.\n";

	return 0;
}

//get next frame and transfer it to a global texture, that's accessible (<IDXGIResource> isn't!)
/**
 * @brief if no next frame is available use the previous analyzed mean-color (disables smoothing) until next frame is available 
 * @return 'true' if frame got captured, oterwise 'false' 
 */
bool get_frame() {
	
	//We want to have the memory in the gpu instead inside the cpu, in wich case we'd drop the memory and end the program
	 if (check_cpu_access_texture() != 0)
		return false;

	 //checking this v description every call is a maybe bit too much
	desktop_duplication->GetDesc(&desktop_duplicate_desc);
	if (desktop_duplicate_desc.DesktopImageInSystemMemory == TRUE) {
		std::cout << "Desktop image is in system memory and this is not want we want.\nAbort..." << "\n";
		exit(EXIT_FAILURE);
		return false;
	} // outsource this check into another loop, before you call get_frame(); ? 
	  // after 15h hours of testing, this ^ if condition was not even once true

	//Release frame directly before acquiring next frame.
	if (new_frame == true)
		new_frame = false;
	hr = desktop_duplication->ReleaseFrame();

#if 1
	if ((sleepTimerMs -8) > 1)
		Sleep(sleepTimerMs -8); //somethings wrong here ! we need a formula to raise the result for lower frame rates ! 
	//give it extra ms, maybe even more^^ 
#endif
	//get accumulated frames
	hr = desktop_duplication->AcquireNextFrame(5/*sleepTimerMs/10*/, &frame_info, &frame); //makig use of win desktop duplication apis' main function
	if (hr == DXGI_ERROR_INVALID_CALL) {
		++fail_invalid;
		return false;
	} if (FAILED(hr) && hr != DXGI_ERROR_INVALID_CALL) {
		++fail_1;
		return false;
	} if (frame_info.AccumulatedFrames == 0) {
		++fail_2;
		return false;
	} if (hr == S_OK) {
		hr = frame->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frame_texture_ori);		
		if (FAILED(hr))
			printf("QueryInterface of 'frame' to 'frame_texture_ori' failed.\n");
		if (frame_texture_ori == nullptr)
			std::cout << "Error: 'frame_texture_ori' is nullptr\n";
		new_frame = true; 
		++acquired_frames_count;  //benchmark
	} 

	context->CopyResource(frame_texture, frame_texture_ori);

	/**
	 * video ram would also work, but a memcpy() to system mem would be necessary. runtime improvement: 0; 
     * best way to do: 
     *  capture in a 2dtexture in gpu ram, "/alternative params/"
     *  obtain a shaderstructure, apply the mipmap-chain. 
     *  memcpy() mipmaps to cpu acces structure. obtain mean-value. 
     * but this way works just fine for ~60fps:
	 */
    //UINT subresource = D3D11CalcSubresource(0, 0, 0);
	hr = context->Map(frame_texture, /*subresource*/0, D3D11_MAP_READ_WRITE /*D3D11_MAP_WRITE_DISCARD*/, 0, &mapped_subresource);
	if (S_OK != hr) {
		printf("Error: Failed to map the pointer of 'frame_texture' to 'mapped_subresource'.\n");
		return false;
	} if (SUCCEEDED(hr))
		++mapped_frames_counter;  //benchmark
	context->Unmap(frame_texture, 0);

#if 0  //#####################################################################################
	//save resource to file to see, if the screen got REALLY captured
	//SO.: code-project.org
	BITMAPINFO	lBmpInfo;

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

	return new_frame;
}

/**
 * @brief test the brightness of the current pixel (after gamma correction)
 * @param curr_pixel 
 * @return true, if pixel is too dark or too white and needs to be ignored.
*/
bool reject_sub_pixel(Pixel &curr_pixel) {
	if (min_brightness_per_pixel == 0 && min_saturation_per_pixel == 0)
		return false;
	else
		return ( !( (curr_pixel.b >= min_brightness_per_pixel) || (curr_pixel.g >= min_brightness_per_pixel) || (curr_pixel.r >= min_brightness_per_pixel) ) || ( (abs(curr_pixel.r - curr_pixel.b) < min_saturation_per_pixel) && (abs(curr_pixel.b - curr_pixel.r) < min_saturation_per_pixel) ) );
	//((curr_pixel.b < 150) && (curr_pixel.g < 150) && (curr_pixel.r < 150)) || ( round(curr_pixel.b / 10.0) == round(curr_pixel.g / 10.0) == round(curr_pixel.r / 10.0) )
}

#if 0
//TODO
std::vector<std::vector<uint8_t>> setup_gamma(){
    std::vector<std::vector<uint8_t>> gamma(256, std::vector<uint8_t>(3, 0));
    // Pre-compute gamma correction table for LED brightness levels:
    for(int i = 0; i < 256; ++i) {
      float f = pow((float)i / 255.0, 2.8);
      gamma[i][0] = (uint8_t)(f * 255.0);
      gamma[i][1] = (uint8_t)(f * 240.0);
      gamma[i][2] = (uint8_t)(f * 220.0);
    }
	return gamma; 
}

//TODO
void adjust_pixel(Pixel &mean_pixel) {
    mean_pixel.b ;
	mean_pixel.g ;
	mean_pixel.r;
	
	return; 
}
#endif

/**
 * @brief retrieve pixel data and calculate the mean of the latest frame directly
 * @param mapped_subresource 
 * @return new_pixel mean b,g,r,a values of the current frame
 */
Pixel retrieve_pixel(D3D11_MAPPED_SUBRESOURCE &mapped_subresource) {
	
	const uint16_t height = texture_desc.Height;
	const uint16_t width = texture_desc.Width;

	//point to bytes/values of pixel data 
	uint8_t* pixel_array_source = static_cast<uint8_t*>(mapped_subresource.pData);

	accum_pixel = { 0, 0, 0 };
	int pixel_amount = 0;
	for (UINT row = 0; row < height; row = row + 2) {							    //+2 instead of ++ drops half the resolution
		UINT row_start = row * mapped_subresource.RowPitch / 4;
		for (UINT col = 0; col < width; col = col + 4) {						    //

			curr_pixel.b = pixel_array_source[row_start + col * 4 + 0];             //first byte = b, according to "DXGI_FORMAT_B8G8R8A8_UNORM"
			curr_pixel.g = pixel_array_source[row_start + col * 4 + 1];
			curr_pixel.r = pixel_array_source[row_start + col * 4 + 2];
            
			if (reject_sub_pixel(curr_pixel)) 
				continue;

			accum_pixel.b += curr_pixel.b;
			accum_pixel.g += curr_pixel.g;
			accum_pixel.r += curr_pixel.r;

			++pixel_amount;
		}
	} 

	UINT8 zero = 0; 
	if (pixel_amount == 0)															//avert division by zero 
		mean_pixel = mean_color_old;												//..mh nothing found, let's send old frame and fade once more..
		//return mean_pixel = { 0, 0, 0 };											//send_data(): does nothing, if 'black'
		//return mean_pixel = {10, 0, 30};											//Default lila for dark scenes

	mean_pixel = {    //gamma adjustment
		accum_pixel.b == zero ? zero : gamma8[((accum_pixel.b - (accum_pixel.b / 3)) / pixel_amount +1/*+ (accum_pixel.b % pixel_amount != zero)*/)],					//blue is a bit too intense with WS2812b ICs on 5050LEDs
		accum_pixel.g == zero ? zero : gamma8[(accum_pixel.g / pixel_amount +1/*+ (accum_pixel.g % pixel_amount != zero)*/)],											//integer ceiling; avoid 0 divising
		accum_pixel.r == zero ? zero : gamma8[(accum_pixel.r / pixel_amount +1/*+ (accum_pixel.r % pixel_amount != zero)*/)],
	};

	return mean_pixel;
}

/**
 * @brief fades the old color with the newly obtained one, to got not that instantaneoulsy changing lights.
 * @param mean_pixel_new the new average color
 * @return mean_pixel_fade the faded color
 */
Pixel fade(Pixel &mean_pixel) {
	Pixel mean_pixel_faded = {
		(mean_pixel.b * (256 - fade_val) + mean_color_old.b * fade_val) >> 8,
		(mean_pixel.g * (256 - fade_val) + mean_color_old.g * fade_val) >> 8,
		(mean_pixel.r * (256 - fade_val) + mean_color_old.r * fade_val) >> 8
	};

	return mean_pixel_faded;
}


/**
 * @brief connect the program with the micro controller 
 * @return bool connected
 */
bool connection_setup(){
	std::cout << "Trying to connect with the LED controller\n\t...\n";
	SP = new Serial(serial_port);
	if (SP->IsConnected())
		std::cout << "Connection established! Let in the light!\n";
		
	return SP->IsConnected();
}

/**
 * @brief send byte array to the micro controller. The first two bytes are the signature bytes. RGB gets attached. 
 * @return true, if send_data() worked or frame was 'black'
 */ 
bool send_data(Pixel &mean_color_new){
	if (mean_color_new.r == 0 && mean_color_new.g == 0 && mean_color_new.b == 0)
		return true;
	uint8_t buffer[5] = {'m', 'o', (uint8_t)mean_color_new.r, (uint8_t)mean_color_new.g, (uint8_t)mean_color_new.b};
	//Sleep(1);
	return SP->WriteData(buffer, 5 /*sizeof(buffer)*/);

}


//#############################################################################################
//##################################### M A I N ###############################################
//###################################### v0.9 #################################################
int main() {
//setup()	
	chosen_output_num = check_monitor_devices(); //
	if (chosen_output_num < 0 || chosen_output_num >= outputs.size()) {
		std::cout << "\n(main1): Something went terribly wrong. -->Exit" << "\n";
		return 1;
	}

	set_sleepTimerMs(fps);
	std::string configurate = "n";
	std::cout << "Would you like to configure the settings? (y/n):\n";
	std::cin.clear();
	std::cin >> configurate;
	if (configurate == "y" || configurate == "Y") {
		std::cout << "\nWhich framerate would you like to capture?\n\tType your (positive) number, or '0' to catch 'em all: \n";
		std::cin.clear();
		std::cin >> fps;
		set_sleepTimerMs(fps);
		std::cout << "sleepTimerMs: " << sleepTimerMs << "\n";
		std::cin.clear();
		int sat, bright = 0;
		std::cout << "Insert min. saturation level of the pixel for the analyzation (0 - 255)):\t\tDefault: 20\n";
		std::cin.clear();
		std::cin >> sat;
		min_saturation_per_pixel = sat;
		std::cin.clear();
		std::cout << "Insert min. brightness level of the pixel for the analyzation (0 - 255)):\t\tDefault: 100\n";
		std::cin >> bright;
		min_brightness_per_pixel = bright;
		std::cin.clear();
		std::cout << "Insert fading factor from one frame to another (0 - 255):\t\tDefault: 100\n\n";
		std::cin >> fade_val;
	}

	create_and_get_device(chosen_output_num);		 //not finished... how do i pass a specific device+monitor to CreateDevice()? 

	if (!connection_setup())
		return -123;
	Sleep(4500);
//()putes

	//30fps * 10sec = 300 frames max.
	int get_frame_call = 0;
	auto start = std::chrono::steady_clock::now();

	while (true) {
		//if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) //runtime
			//break;

		mean_color_old = mean_color_new;
		if (get_frame()) { 							//try no if condition here for smoother lights when less than 24fps, but adjust send_data with a 1ms sleep..
			mean_color_new = retrieve_pixel(mapped_subresource);

			mean_color_new = fade(mean_color_new);
			if (!send_data(mean_color_new)) {		//send data to micro controller
				std::cout << "Sending data to the micro controller failed!\n\tTrying to reconnect...\n";
				Sleep(5000);
				connection_setup();
				Sleep(5000);
			}
		}
			
		if (mapped_frames_counter % (fps+9) == 0)
			std::cout << "mean_color_new: " << mean_color_new.r << "r " << mean_color_new.g << "g " << mean_color_new.b << "b\n";
		++get_frame_call;
	}

	//simple benchmark
	std::cout << "\n\nget_frame() got " << get_frame_call << " times called\n";
	std::cout << "Aquired_frames_count: " << acquired_frames_count << "\n";
	std::cout << "mapped_frames_counter: " << mapped_frames_counter << "\n";
	std::cout << "fail_invalid: " << fail_invalid << "\n";
	std::cout << "fail_1: " << fail_1 << "\n";
	std::cout << "fail_2: " << fail_2 << "\n";
	std::cin.clear();
	std::cout << "Press 'Enter' to end! \n";
	std::cin.ignore();

	std::string end = "";

	//oupsie:
	//TODO: Clean Up !

	return 0;
}