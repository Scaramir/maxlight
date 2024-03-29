/**
 * @file main.cpp
 * @author Maximilian Otto (maxotto45@gmail.com)
 * @brief This is used to control an arduino with an LED strip attached to it. www.github.com/Scaramir/maxlight
 * @version 1.0
 * @date 2021-10-15
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <iostream>
#include <sstream> 
#include <dxgi1_2.h>  // include-order is important
#include <d3d11.h>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <conio.h>
#include <chrono>
#include <shlobj.h>
#include <shellapi.h>

#include "./include/ccomptrcustom_class.hpp"
#include "./include/SerialClass.h"

#pragma comment(lib, "dxgi") // this is for CreateDXGIFactory1()
#pragma comment(lib, "d3d11")


//#############################################################################################
// Driver types supported
D3D_DRIVER_TYPE gDriverTypes[] = { D3D_DRIVER_TYPE_HARDWARE };
UINT gNumDriverTypes = ARRAYSIZE(gDriverTypes);

// Customized DirectX feature level support
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
// Declaration of global variables:
namespace screen_capture {
	// main:
	static int32_t sleepTimerMs = (int)(((float)1 / 33) * 1000);
	static int32_t fps = 30;
	void set_sleepTimerMs(int& fps) { if (fps == 0) sleepTimerMs = 0; else sleepTimerMs = ((int)((1. / fps) * 1000) - 1); }
	HRESULT hr = E_FAIL;
	// output_enumeration() + check_monitor_devices()
	std::vector<IDXGIAdapter1*> adapters;								    // Needs to be Released()
	static uint8_t chosen_adapter_num = 0;									// default adpater 
	IDXGIAdapter1* chosen_adapter = nullptr;
	std::vector<IDXGIOutput*> outputs;
	static int chosen_output_num = 0;
	static DXGI_OUTPUT_DESC desc;  
	// check_cpu_access()
	D3D11_TEXTURE2D_DESC texture_desc;
	// create_and_get_device()
	CComPtrCustom<ID3D11Device> device = nullptr;
	CComPtrCustom<ID3D11DeviceContext> context = nullptr;
	CComPtrCustom<IDXGIOutputDuplication> desktop_duplication = nullptr;
	// reject_sub_pixel()
	static uint8_t min_saturation_per_pixel = 7;							
	static uint8_t min_brightness_per_pixel = 20;
	// get_frame()
	CComPtrCustom<ID3D11Texture2D> frame_texture = nullptr;
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	// benchmark
	static uint32_t mapped_frames_counter = 0;
	// arduino connection 
	//const char* serial_port = "COM7";										// usb port name 
	Serial* SP;
	// led_stuff:
	struct Pixel {
	public:
		uint8_t b = 0;	                   									// initialized to 'black'; 
		uint8_t g = 0;
		uint8_t r = 0;
	};
	// fade:
	int fade_val = 185;														// default value
	Pixel mean_color_old_l;
	Pixel mean_color_old_r;
	Pixel mean_color_new_l;
	Pixel mean_color_new_r;

	const uint8_t gamma8_neo_pixel[256] = {
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
			1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
			2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,
			4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   6,
			6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,   10,  10,  10,
			11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,
			17,  18,  18,  19,  19,  20,  20,  21,  21,  22,  22,  23,  24,  24,  25,
			25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33,  34,  34,  35,
			36,  37,  38,  38,  39,  40,  41,  42,  43,  43,  44,  46,  47,  48,  49,
			50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
			65,  66,  67,  69,  70,  71,  72,  73,  74,  75,  77,  78,  79,  81,  82,
			83,  85,  86,  87,  89,  90,  92,  93,  94,  95,  96,  98, 100,  101, 103,
			104, 106, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 124, 126,
			128, 130, 131, 133, 135, 137, 138, 140, 142, 144, 146, 147, 149, 151, 153,
			155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 182, 183,
			185, 187, 189, 192, 194, 196, 198, 200, 203, 205, 207, 210, 212, 214, 216,
			219, 221, 224, 226, 228, 231, 233, 236, 238, 241, 243, 246, 248, 251, 253, 
			255};
};
using namespace screen_capture;
//#############################################################################################

// Print every char of a string to the console with a short dely between each char
/**
 * @brief unrolling text on terminal
 * @param text to print
 * @param color-value default=10 (green), red would be '12'.
 */
void terminal_fill(std::string_view text, int8_t color = 10, int delay = 8) {
	if (text.empty())
		return;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE)
		std::cout << text << std::endl;
		return;
	SetConsoleTextAttribute(hConsole, color);
	for (char c : text) {
		std::cout << c;
		Sleep(delay);
	}
	return;
}

// Check devices for outputs and enumerate them
/**
 * @brief check monitors (outputs) for a graphic adpater and push it into vec<output>
 * @param i index of adapter
 * @return
 */
int output_enumeration(int16_t& i) {
	int dx = 0;
	CComPtrCustom<IDXGIOutput> output = nullptr;

	while (DXGI_ERROR_NOT_FOUND != adapters[i]->EnumOutputs(dx, &output)) {
		int monitor_num = outputs.size();	// current monitor index
		std::cout << "\t  (" << monitor_num << ".) ";
		printf("Found monitor %d on adapter: %lu \n", monitor_num, i);
		outputs.push_back(output);			// store the found monitor

		HRESULT hr = outputs[monitor_num]->GetDesc(&desc);
		if (SUCCEEDED(hr)) {				// print info
			wprintf(L"\t\tMonitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'Y' : 'n');
			if (desc.Rotation == DXGI_MODE_ROTATION_IDENTITY 
				|| desc.Rotation == DXGI_MODE_ROTATION_UNSPECIFIED 
				|| desc.Rotation == DXGI_MODE_ROTATION_ROTATE180) {
				wprintf(L"\t\tMonitor is in a horizontal mode\n");
				std::cout << "\t\tand has the following dimensions:\n\t\t " <<
					abs(abs((int)desc.DesktopCoordinates.right) - abs((int)desc.DesktopCoordinates.left)) <<
					" x " <<
					abs(abs((int)desc.DesktopCoordinates.top) - abs((int)desc.DesktopCoordinates.bottom)) <<
					" pixel" << "\n";
			} else {
				wprintf(L"\t\tMonitor is in a vertical mode\n");
				std::cout << "\t\tand has the following dimensions:\n\t\t " <<
					abs(abs((int)desc.DesktopCoordinates.right) - abs((int)desc.DesktopCoordinates.left)) <<
					" x " <<
					abs(abs((int)desc.DesktopCoordinates.bottom) + abs((int)desc.DesktopCoordinates.top)) <<
					" pixel" << "\n";
			}
		} else {
			terminal_fill("Error: failed to retrieve a DXGI_OUTPUT_DESC for output " + std::to_string(i) + ".\nContinue\n", 12);
			continue;
		}
		++monitor_num; ++dx;
	}
	return dx;
}

// Check for devices/adapters, monitors, and choose one
/**
 * @brief check for adapters and devices; choose one
 * @return chosen_output_num
 */
int check_monitor_devices() {
	HRESULT hr = E_FAIL;

	// Create device that's able to enumerate adapters
	IDXGIFactory1* factory = nullptr;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory)); // winSDK and x64 needed for compilation
	if (FAILED(hr)) {
		terminal_fill("Error: failed to retrieve the IDXGIFactory.\n", 12);
		return -1;
	}

	// Enumerate the adapters aka GPUs
	IDXGIAdapter1* adapter = nullptr;
	INT16 i = 0;
	while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter)) {
		adapters.push_back(adapter);
		++i;
	}
	if (adapters.empty()) {
		terminal_fill("Error: no adapter found. Enumaration fails accordingly. \n\tSomething went really wrong! ...\n", 12);
		return -1;
	}

	// Print list of adapters and push corresponding outputs
	terminal_fill("Following graphics adapters (GPUs) are found:\n");
	for (INT16 i = 0; i < adapters.size(); ++i) {
		DXGI_ADAPTER_DESC1 desc;
		hr = adapters[i]->GetDesc1(&desc);

		if (SUCCEEDED(hr)) {
			wprintf(L"Adapter: %lu, description: %s\n", i, desc.Description);
			int monitors_on_adapter = output_enumeration(i);
			std::cout << "\t" << monitors_on_adapter << " monitor(s) found.\n\n";
		} else {
			terminal_fill("\tError: failed to get a description for the adapter: " + std::to_string(i) + "\n  Please check your drivers.\n", 12);
			continue;
		}
	}

	// select monitor and adapter for analyzation
	terminal_fill("\nWhich monitor do you choose?\n\tType the number (x.) of the monitor.\n", 13);
	std::cin.clear(); //yaya, use scanl, getline, anything but cin 
	std::cin >> chosen_output_num;
	// fix device creation for this:
	//std::cout << "Which graphics adapter do you choose?\n\tType the number of adapter." << "\n";
	std::cin.clear();
	//std::cin >> chosen_adapter_num;

	return chosen_output_num;
}

/**
 * @brief creates a cpu-accessible texture (D3D11Texture2D); this way, the texture can get copied and mapped.
 * @param frame_texture where the frame gets saved (memory-wise)
 * @return false, if texture is not accessible or could not be created
 */
bool check_cpu_access_texture(CComPtrCustom<ID3D11Texture2D>& frame_texture) {
	//create a texture with cpu_access_read
	//we want to copy the texture to an empty texture
	if (frame_texture != nullptr)
		return false;

	D3D11_SUBRESOURCE_DATA sub_data = {
		sub_data.pSysMem = std::calloc((long long int)texture_desc.Width * texture_desc.Height, 4),
		sub_data.SysMemPitch = 4 * texture_desc.Width,
		sub_data.SysMemSlicePitch = 0
	};

	HRESULT hr = device->CreateTexture2D(&texture_desc, &sub_data, &frame_texture);
	if (FAILED(hr)) {
		terminal_fill("Error: Failed to create the 2DTexture.\n", 12);
		return false;
	}

	return true;
}

//create D3DX-Device and query interfaces 
/**
 * @brief create device (D3D11) for @chosen_monitor
 * @param chosen_monitor number of the monitor (retrieved of @outputs)
 * @return success (0) or fail (negativ)
 */
int create_and_get_device(int& chosen_monitor) {

	D3D_FEATURE_LEVEL feature_level;
	CComPtrCustom<IDXGIOutput1> output1 = nullptr;
	IDXGIOutput* chosen_output = 0;

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
			break;
		}
	}

	if (hr == E_INVALIDARG) {                
		hr = D3D11CreateDevice(
			nullptr,                         // outputAdapter
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			&gFeatureLevels[1],
			gNumFeatureLevels - 1,
			D3D11_SDK_VERSION,
			&device,                         // TODO: enter specific graphic device, adapter and monitor in this section
			&feature_level,
			&context
		);
		if (SUCCEEDED(hr))
			printf("Device creation succesful (feature level: D3D11).\n");
	}
	if (FAILED(hr) || device == nullptr) {
		terminal_fill("Error: failed to create a D3D11 Device and Context.\n", 12);
		context.Release();
		return -1;
	}

	/**
	 * Create a IDXGIOutputDuplication
	 * query a IDXGIOutput1, because the IDXGIOutput1 has the DuplicateOutput feature.
	 */
	chosen_output = outputs[chosen_output_num];
	chosen_output->GetDesc(&desc);
	if (chosen_output == nullptr) {
		terminal_fill("Selected Monitor '" + std::to_string(chosen_output_num) + "' is invalid.\n", 12);
		return -2;
	}

	hr = chosen_output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
	if (FAILED(hr)) {
		terminal_fill("Error: QueryInterface() failed.\n", 12);
		return -3;
	}
	chosen_output->Release();

	desktop_duplication.Release();
	hr = output1->DuplicateOutput(device, &desktop_duplication);
	if (FAILED(hr)) {
		terminal_fill("Error: Desktop duplication failed.\n", 12);
		return -4;
	}
	output1.Release();

	//Initilize texture descriptions
	DXGI_OUTDUPL_DESC desktop_duplicate_desc;
	desktop_duplication->GetDesc(&desktop_duplicate_desc);
	ZeroMemory(&texture_desc, sizeof(texture_desc));
	
	if (desc.Rotation == DXGI_MODE_ROTATION_IDENTITY || desc.Rotation == DXGI_MODE_ROTATION_UNSPECIFIED || desc.Rotation == DXGI_MODE_ROTATION_ROTATE180) {
		texture_desc.Width = desktop_duplicate_desc.ModeDesc.Width;
		texture_desc.Height = desktop_duplicate_desc.ModeDesc.Height;
	}
	else {
		texture_desc.Width = desktop_duplicate_desc.ModeDesc.Height;
		texture_desc.Height = desktop_duplicate_desc.ModeDesc.Width;
	}
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the default data when using desktop duplication
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_STAGING /*D3D11_USAGE_DYNAMIC*/; // comments: this would lead to a GPU accessible texture only. --> TODO
	texture_desc.BindFlags = 0 /*D3D11_BIND_SHADER_RESOURCE*/;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; 
	texture_desc.MiscFlags = 0;
	std::cout << "Capturing a " << texture_desc.Width << " x " << texture_desc.Height << " monitor.\n";

	//Can the cpu read the texture?
	if (!check_cpu_access_texture(frame_texture)) {		
		terminal_fill("The used texture is not accessible by the cpu. ", 12);
		return -100;
	}
	
	terminal_fill("\n--- Setup completed ---\n");
	return 0;
}

//get next frame and transfer it to a global texture, that's accessible (<IDXGIResource> isn't!)
/**
 * @brief if no next frame is available use the previous analyzed mean-color (disables smoothing) until next frame is available
 * @return 'true' if frame got captured, oterwise 'false'
 */
bool get_frame() {
	bool new_frame = false;
	CComPtrCustom<IDXGIResource> frame = nullptr;
	CComPtrCustom<ID3D11Texture2D> frame_texture_ori = nullptr;
	DXGI_OUTDUPL_FRAME_INFO frame_info;

	#if 0
	// We want to have the memory in the GPU. Otherwise, we'd drop the memory and end the program
	// checking this v description every call is a maybe bit too much
	DXGI_OUTDUPL_DESC desktop_duplicate_desc;
	desktop_duplication->GetDesc(&desktop_duplicate_desc);
	if (desktop_duplicate_desc.DesktopImageInSystemMemory == TRUE) {
		std::cout << "Desktop image is in system memory and this is not want we want.\n  ..." << "\n";
		return false;
	}
	// outsource this check into another loop, before you call get_frame(); ? 
	// Edit: after 15h hours of testing, this ^ if condition was not even once true
	#endif

	// Release frame directly before acquiring next frame.
	hr = desktop_duplication->ReleaseFrame();

	// let some frames get accumulated 
	if ((sleepTimerMs - 15) > 0)
		Sleep(sleepTimerMs - 15);
	// this value is specific for my system 
	// just give it some extra ms

	//get accumulated frames
	hr = desktop_duplication->AcquireNextFrame(100, &frame_info, &frame); // win desktop duplication apis' core function
	if (hr == DXGI_ERROR_INVALID_CALL) {
		return false;
	} if (hr == E_INVALIDARG) {
		return false;
	} if (frame_info.AccumulatedFrames == 0) {
		return false;
	} if (FAILED(hr) && hr != DXGI_ERROR_WAIT_TIMEOUT) {				// ^= (hr == DXGI_ERROR_ACCESS_LOST)
		desktop_duplication->ReleaseFrame();
		desktop_duplication.Release();
		device.Release();
		context.Release();
		create_and_get_device(chosen_output_num);
		return false;
	} if (hr == S_OK) {
		hr = frame->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&frame_texture_ori);
		if (FAILED(hr))
			terminal_fill("QueryInterface of 'frame' to 'frame_texture_ori' failed.\n", 12);
		if (frame_texture_ori == nullptr)
			terminal_fill("Error: 'frame_texture_ori' is nullptr\n", 12);
		new_frame = true;
		frame.Release();
	}

	// Copy to destination texture from source texture
	context->CopyResource(frame_texture, frame_texture_ori);

	// Map the texture for CPU access 
	hr = context->Map(frame_texture, 0, D3D11_MAP_READ_WRITE /*D3D11_MAP_WRITE_DISCARD*/, 0, &mapped_subresource);
	if (S_OK != hr) {
		terminal_fill("Error: Failed to map the pointer of 'frame_texture' to 'mapped_subresource'.\n", 12);
		return false;
	} if (SUCCEEDED(hr))
		++mapped_frames_counter;  // benchmark
	context->Unmap(frame_texture, 0);

	return new_frame;
}

/**
 * @brief test the brightness and saturation of the current pixel.
 * @param curr_pixel
 * @return true, if pixel is too dark or too white and needs to be ignored.
*/
bool reject_sub_pixel(const Pixel& curr_pixel) {
	if (min_brightness_per_pixel == 0 && min_saturation_per_pixel == 0)
		return false;

	if (min_brightness_per_pixel > 0) {
		if ((curr_pixel.b < min_brightness_per_pixel) && 
			(curr_pixel.g < min_brightness_per_pixel) && 
			(curr_pixel.r < min_brightness_per_pixel))
			return true;
	}

	if (min_saturation_per_pixel > 0) {
		if ((abs(curr_pixel.r - curr_pixel.g) < min_saturation_per_pixel) && 
			(abs(curr_pixel.g - curr_pixel.b) < min_saturation_per_pixel) && 
			(abs(curr_pixel.r - curr_pixel.b) < min_saturation_per_pixel))
			return true;
	}
	return false;
}	


/**
 * @brief retrieve pixel data and calculate the mean of the latest frame directly
 * @param mapped_subresource: pointer to the texture
 * @param side: '1' for left side, '2' for right side
 * @return new_pixel mean b,g,r,a values of the current frame
 */
Pixel retrieve_pixel(D3D11_MAPPED_SUBRESOURCE& mapped_subresource, const int& side) {
	// point to bytes/values of pixel data 
	uint8_t* pixel_array_source = static_cast<uint8_t*>(mapped_subresource.pData);
	uint8_t curr_b = 0;
	uint8_t curr_g = 0;
	uint8_t curr_r = 0;
	uint32_t accum_pixel_b = 0;
	uint32_t accum_pixel_g = 0;
	uint32_t accum_pixel_r = 0;
	Pixel mean_pixel = {0, 0, 0};
	uint32_t curr_pixel_position = 0;

	/** TODO: 
	 *  change row&col start and row&col end acoridng to given coordinates
	 * 	modify loop to an row < row_end
	 *  this way, we aren't limited to just two sides and can run multiple instances or this function
	 * 	  to analyze just the pixels within the given coordinates. 
	 *    therefore, the support for a horizontal mode would be also important 
	 */

	uint32_t pixel_amount = 0;
	uint32_t col_start = (side == 1) ? 0 : (texture_desc.Width/2) - texture_desc.Width / 10; //middle overlap 8
	uint32_t col_end = (side == 1) ? (texture_desc.Width / 2) + texture_desc.Width / 10 : texture_desc.Width;
	for (uint32_t row = 0; row < texture_desc.Height; row += 3) {                // +2 instead of ++ drops half the resolution
		uint32_t row_start = row * mapped_subresource.RowPitch;	                 // usually it's RGBA(unsigned Char) but we only care for rgb, 'cause a is always 255, 
		for (uint32_t curr_col = col_start; curr_col < col_end; curr_col += 4) { // col + quality_loss 
			curr_pixel_position = row_start + (curr_col * 4);
			curr_b = pixel_array_source[curr_pixel_position];					 // first byte = b, according to "DXGI_FORMAT_B8G8R8A8_UNORM"
			curr_g = pixel_array_source[curr_pixel_position + 1];
			curr_r = pixel_array_source[curr_pixel_position + 2];

			if (reject_sub_pixel({ curr_b, curr_g, curr_r }))
				continue;

			accum_pixel_b += curr_b;
			accum_pixel_g += curr_g;
			accum_pixel_r += curr_r;
			++pixel_amount;
		}
	}

	if (pixel_amount == 0) {													 // avert division by zero 
		mean_pixel = (side == 1) ? mean_color_old_l : mean_color_old_r;			 // ..mh nothing found, let's send old frame and fade once more..
		return mean_pixel;
	}

	mean_pixel = {    	// calculate mean of all pixels
		static_cast<uint8_t>(accum_pixel_b / pixel_amount),
		static_cast<uint8_t>(accum_pixel_g / pixel_amount),
		static_cast<uint8_t>(accum_pixel_r / pixel_amount)
	};

	return mean_pixel;
}

/**
 * @brief fades the old color with the newly obtained one, to get not that drastically changing lights.
 * @param mean_pixel_new the new average color
 * @return mean_pixel_fade the faded color
 */
Pixel fade(const Pixel &pixel, const Pixel &mean_color_old){
	uint8_t inv_fade_val = 256 - fade_val;
	Pixel pixel_faded;
	pixel_faded.b = (pixel.b * inv_fade_val + mean_color_old.b * fade_val) >> 8;
	pixel_faded.g = (pixel.g * inv_fade_val + mean_color_old.g * fade_val) >> 8;
	pixel_faded.r = (pixel.r * inv_fade_val + mean_color_old.r * fade_val) >> 8;
	return pixel_faded;
}

/**
 * @brief grabs the gamma corrected values for each color out of ::gamma8[]
 * @param pixel 
 * @return Pixel after gamma_correction 
 */
Pixel gamma_correction(const Pixel &pixel){
	Pixel pixel_gamma_corrected = {
		gamma8_neo_pixel[pixel.b == 0 ? 0 : (uint8_t)ceil(pixel.b - (pixel.b / 5.))],
		gamma8_neo_pixel[pixel.g == 0 ? 0 : (uint8_t)ceil(pixel.g - (pixel.b / 17.))],
		gamma8_neo_pixel[pixel.r]};

	return pixel_gamma_corrected;
}

// true gamme correction:
// TODO: create the lookup table, like this, cache it, an adjust it 
// 		 add a modified version of gamma_correction()! 
#if 0
std::vector<std::vector<uint8_t>> setup_gamma(float gamma_value=2.8) {
	std::vector<std::vector<uint8_t>> gamma(256, std::vector<uint8_t>(3, 0));
	// Pre-compute gamma correction table for LED brightness levels:
	for (int i = 0; i < 256; ++i) {
		float f = pow((float)i / 255., gamma_value);
		gamma[i][0] = (uint8_t)(f * 255.);
		gamma[i][1] = (uint8_t)(f * 240.);
		gamma[i][2] = (uint8_t)(f * 220.);
	}

	return gamma;
}
#endif

/**
 * @brief connect the program with the micro controller
 * @return bool connected
 */
bool connection_setup() {
	terminal_fill("\rTrying to connect with the LED controller\r   ...                                    \r");

	std::string s = "COM0";
	for (int i = 0; i < 1000; ++i) {
		if (i < 10)
			s[3] = (char)(i+48);
		else
			s = "\\\\.\\COM" + std::to_string(i);
		const char* c = s.c_str();
		SP = new Serial(c);
		if (SP->IsConnected()) {
			terminal_fill("Connection established! Let in the light!\n");
			break;
		}
		delete SP;
	}
	return SP->IsConnected();
}

/**
 * @brief send byte array to the micro controller. The first two bytes are the signature bytes. RGB8 gets attached.
 * @param pixel, the new color values to be sent.
 * @return true, if send_data() worked or frame was 'black'.
 */
bool send_data(const Pixel& pixel_l, const Pixel& pixel_r, const char header1 = 'm', const char header2 = 'o') {
	uint8_t buffer[8] = { header1, header2, pixel_l.r, pixel_l.g, pixel_l.b, pixel_r.r, pixel_r.g, pixel_r.b };
	return SP->WriteData(buffer, 8 /*sizeof(buffer)*/);
}

/**
 * @brief performance check of the system using different sleep timers.
 * @return true, if performance check got executed.
 */
bool setup_and_benchmark() {
	terminal_fill("\nBegin a performance check? ('y' / 'n'):\n\tHint: Start a high-fps video or stream before you type 'y'\n", 14);
	std::cin.clear();
	std::string bench;
	std::cin >> bench;
	if (bench != "y") {
		if (!connection_setup())
			return false;
		Sleep(5000);
		create_and_get_device(chosen_output_num);		 	// not finished... how do i pass a specific device+monitor to CreateDevice() for different grakas and rotations?
		return true;
	}
	if (!connection_setup())
		return false;
	Sleep(3000);
	create_and_get_device(chosen_output_num);

	terminal_fill("I will run a test for 50 seconds, now!\n", 14);
	int user_fps = fps;
	std::vector<int> fps_vec = { user_fps, 25, 30, 60, 0 };
	for (size_t i = 0; i < fps_vec.size(); ++i) {
		terminal_fill("\n--- Checking for max. " + std::to_string((int)fps_vec[i]) + "fps: ---\n", 14);

		int get_frame_call = 0;
		mapped_frames_counter = 0;
		fps = fps_vec[i];
		set_sleepTimerMs(fps);

		auto start = std::chrono::steady_clock::now();
		while (true) {
			if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) // runtime
				break;
			mean_color_old_l = mean_color_new_l; // used in 'fade()'
			mean_color_old_r = mean_color_new_r; // used in 'fade()'
			if (!get_frame())					 // try no if-condition here for smoother lights when less than 24fps, but adjust send_data with a 1m sleep..
				continue;
			int i = 1; // Left side first
			mean_color_new_l = retrieve_pixel(mapped_subresource, i);
			mean_color_new_l = fade(mean_color_new_l, mean_color_old_l);
			Pixel mean_color_gamma_corrected_l = gamma_correction(mean_color_new_l);
			++i; // Right side second
			mean_color_new_r = retrieve_pixel(mapped_subresource, i);
			mean_color_new_r = fade(mean_color_new_r, mean_color_old_r);
			Pixel mean_color_gamma_corrected_r = gamma_correction(mean_color_new_r);

			if (!send_data(mean_color_gamma_corrected_l, mean_color_gamma_corrected_r)) {									// send data to micro controller
				terminal_fill("Sending data to the micro controller failed!\r\tTrying to reconnect...                          \r", 12);
				Sleep(5000);
				connection_setup();
				Sleep(5000);
			}
			++get_frame_call;
		}
		terminal_fill("I captured: ~" + std::to_string(get_frame_call / 10) + " fps.\n", 14, 6);
	}
	set_sleepTimerMs(user_fps); 		// resettimer

	return true;
}

bool configuration() {
	set_sleepTimerMs(fps);
	std::string configurate = "n";
	terminal_fill("\nWould you like to configure the settings? (y/n):\n", 11);
	std::cin.clear();
	std::cin >> configurate;
	if (configurate == "y" || configurate == "Y") {
		terminal_fill("\nWhich framerate would you like to capture?\n\tType your (positive) number, or '0' to catch 'em all:\t\tDefault: " + std::to_string(fps) + "\n", 11, 6);
		std::cin.clear();
		std::cin >> fps;
		set_sleepTimerMs(fps);
		std::cin.clear();

		int sat, bright = 0;
		terminal_fill("Insert min. saturation level of the pixel for the analyzation (0 - 255)):\t\tDefault: " + std::to_string(min_saturation_per_pixel) + "\n", 11, 6);
		std::cin.clear();
		std::cin >> sat;
		min_saturation_per_pixel = sat;

		terminal_fill("Insert min. brightness level of the pixel for the analyzation (0 - 255)):\t\tDefault: " + std::to_string(min_brightness_per_pixel) + "\n", 11, 6);
		std::cin.clear();
		std::cin >> bright;
		min_brightness_per_pixel = bright;

		terminal_fill("Insert fading factor from one frame to another (0 - 255):\t\tDefault: " + std::to_string(fade_val) + "\n", 11, 6);
		std::cin.clear();
		std::cin >> fade_val;
	}
	terminal_fill("\n--- Proceeding with following settings: ---\n");
	std::cout << "\tMin. Saturation per Pixel: " << (int)min_saturation_per_pixel << "\n";
	std::cout << "\tMin. Brightness per Pixel: " << (int)min_brightness_per_pixel << "\n";
	std::cout << "\tFading factor: " << fade_val << "\n";
	std::cout << "\tMax. fps: " << fps << "\n";

	return true;
}

//#############################################################################################
//##################################### M A I N ###############################################
//###################################### v1.0 #################################################
int main() {
	//LPCSTR title = "MaxLight"; 								// for VS Code  
	LPCWSTR title = L"MaxLight"; 								// for VS2019
	SetConsoleTitle(title);
	terminal_fill("--- MaxLight v1.0 --- \n\n\n");

	chosen_output_num = check_monitor_devices();
	if (chosen_output_num < 0 || chosen_output_num >= outputs.size()) {
		terminal_fill("\nError (main1): Something went terribly wrong. --> Exit\n", 12);
		return -1;
	}

	configuration();

	if (!setup_and_benchmark())
		return -2;

	terminal_fill("\n--- Starting continuous analyzation ---\n");

	terminal_fill("    --- Press any key to abort ---\n\n");
	while (true) {
		// abort if input to console is detected
		if (_kbhit()) {
			break;
		}

		mean_color_old_l = mean_color_new_l;  // used in 'fade()'
		mean_color_old_r = mean_color_new_r;  // used in 'fade()'
		if (!get_frame()) 					  // try no if-condition here for smoother lights when less than 24fps, but adjust send_data with a 1m sleep..
			continue;
		
		// Left side first
		mean_color_new_l = retrieve_pixel(mapped_subresource, 1);
		mean_color_new_l = fade(mean_color_new_l, mean_color_old_l);
		Pixel mean_color_gamma_corrected_l = gamma_correction(mean_color_new_l);
		// Right side
		mean_color_new_r = retrieve_pixel(mapped_subresource, 2);
		mean_color_new_r = fade(mean_color_new_r, mean_color_old_r);
		Pixel mean_color_gamma_corrected_r = gamma_correction(mean_color_new_r);

		if (!send_data(mean_color_gamma_corrected_l, mean_color_gamma_corrected_r)) {
			terminal_fill("Error: Sending data to the micro controller failed!\r\tTrying to reconnect...                                 \r", 12);
			Sleep(5000);
			connection_setup();
			Sleep(5000);
		}
		// prints:
		if (mapped_frames_counter % (fps + 1) == 0)
			std::cout << "new_avg_pixel_l: " << (int)mean_color_gamma_corrected_l.r << "r, " 
					<< (int)mean_color_gamma_corrected_l.g << "g, " 
					<< (int)mean_color_gamma_corrected_l.b << "b, " 
					<< (int)mean_color_gamma_corrected_r.r << "r_r, " 
					<< (int)mean_color_gamma_corrected_r.g << "g_r, " 
					<< (int)mean_color_gamma_corrected_r.b << "b_r        \r";
	}

	// raindbow mode:
	if (!send_data(Pixel{ (uint8_t)0, (uint8_t)0, (uint8_t)0 }, Pixel{ (uint8_t)0, (uint8_t)0, (uint8_t)0 }, 'r', 'b')) {
		terminal_fill("Error: Sending data to the micro controller failed!\r\tTrying to reconnect...                                 \r", 12);
		Sleep(5000);
		connection_setup();
		Sleep(5000);
	}

	std::cin.clear();
	std::cout << "\nPress 'Enter' to end!\n";
	std::cin.ignore();
	std::cin.ignore();

	// oupsie:
	// TODO: Clean Up !
	return 0;
}