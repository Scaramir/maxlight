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
#include <dxgi1_2.h>        // include-order
#include <d3d11.h>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <conio.h>
#include <chrono>
#include <shlobj.h>
#include <shellapi.h>

#include "include/ccomptrcustom_class.hpp"
#include "include/SerialClass.h"

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
	UINT sleepTimerMs = (int)(((float)1 / 33) * 1000);
	UINT fps = 35;
	void set_sleepTimerMs(unsigned int& fps) { if (fps == 0) sleepTimerMs = 0; else sleepTimerMs = ((int)(((float)1 / fps) * 1000) - 1); }
	HRESULT hr = E_FAIL;
	// output_enumeration() + check_monitor_devices()
	std::vector<IDXGIAdapter1*> adapters;							// Needs to be Released()
	static int chosen_adapter_num = 0;								// default adpater 
	IDXGIAdapter1* chosen_adapter = nullptr;
	std::vector<IDXGIOutput*> outputs;
	static int chosen_output_num = 0;
	// check_cpu_access()
	D3D11_TEXTURE2D_DESC texture_desc;
	// create_and_get_device()
	CComPtrCustom<ID3D11Device> device = nullptr;
	CComPtrCustom<ID3D11DeviceContext> context = nullptr;
	CComPtrCustom<IDXGIOutputDuplication> desktop_duplication = nullptr;
	// reject_sub_pixel()
	UINT8 min_saturation_per_pixel = 18;							// optional accents: 60;
	UINT8 min_brightness_per_pixel = 40;							//                  160;
// get_frame()
	CComPtrCustom<ID3D11Texture2D> frame_texture = nullptr;
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
// benchmark
	int mapped_frames_counter = 0;
// arduino connection 
	const char serial_port[] = { 'C', 'O', 'M', '7' };			    // usb port name 
	Serial* SP;
// led_stuff:
	struct Pixel {
	public:
		int b = 0;	                   								// initialized to 'black'; 
		int g = 0;
		int r = 0;
	};
// fade:
	int fade_val = 90;												// default value
	Pixel mean_color_old;
	Pixel mean_color_new;

	const uint8_t gamma8[] = {
					0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
					0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
					1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
					2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
					5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
					10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
					17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
					25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
					37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
					52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
					69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
					90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
					115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
					144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
					177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
					215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
};
using namespace screen_capture;
//#############################################################################################

/**
 * @brief unrolling text on terminal
 * @param text to print
 * @param color-value default=10 (green), red would be '12'.
 */
void terminal_fill(std::string s, int c = 10) {
	if (s.empty())
		return;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, c);

	for (char& c : s) {
		std::cout << c;
		Sleep(10);
	}

	return;
}

// Check Devices for outputs
/**
 * @brief check monitors (outputs) for a graphic adpater and push it into vec<output>
 * @param i index of adapter
 * @return
 */
int output_enumeration(INT16& i) {
	int dx = 0;
	CComPtrCustom<IDXGIOutput> output = nullptr;

	while (DXGI_ERROR_NOT_FOUND != adapters[i]->EnumOutputs(dx, &output)) {
		int monitor_num = outputs.size();	// current monitor index
		std::cout << "\t  (" << monitor_num << ".) ";
		printf("Found monitor %d on adapter: %lu \n", monitor_num, i);
		outputs.push_back(output);	// store the found monitor

		DXGI_OUTPUT_DESC desc;		// get description of the monitor
		HRESULT hr = outputs[monitor_num]->GetDesc(&desc);
		if (SUCCEEDED(hr)) {		// print info
			wprintf(L"\t\tMonitor: %s, attached to desktop: %c\n", desc.DeviceName, (desc.AttachedToDesktop) ? 'Y' : 'n');
			std::cout << "\t\twith the following dimensions:\n\t\t " <<
			   abs(abs((int)desc.DesktopCoordinates.right) - abs((int)desc.DesktopCoordinates.left)) <<
				" x " <<
				abs(abs((int)desc.DesktopCoordinates.top) - abs((int)desc.DesktopCoordinates.bottom)) <<
				" pixel" << "\n";
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
			wprintf(L"Adapter: %lu, description: %s\n\t..searching for monitors..\n", i, desc.Description); //wide-character for UNICODE
			int found_on_adapter = output_enumeration(i);
			std::cout << "\t" << found_on_adapter << " monitor(s) found.\n\n";
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
 * @brief creates a cpu access texture (D3D11Texture2D); this way, the texture can get copied and mapped.
 * this can be stored in system memory or as a shader-texture
 * @return false, if texture is not accessible
 */
bool check_cpu_access_texture(CComPtrCustom<ID3D11Texture2D>& frame_texture) {
	//create a texture with cpu_access_read
	if (frame_texture != nullptr)
		return false;
	else
		printf("Creating new 2DTexture\n");

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
	//else
	//	printf("'CreateTexture2D()' was successful!\n");

	return true;
}

//create D3DX-Device and query interfaces 
/**
 * @brief create device (D3D11) for @chosen_monitor
 * @param chosen_monitor number of monitor (retrieve of @outputs)
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
			printf("Device creation successful.\n");
			break;
		}
	}

	if (hr == E_INVALIDARG) {                // in Case D3D11_1 does not work (Error: invalid_arguments passed), take D3D11 and standard notation/parameters
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
	if (chosen_output == nullptr) {
		terminal_fill("Selected Monitor '" + std::to_string(chosen_output_num) + "' is invalid.\n", 12);
		return -2;
	}

	hr = chosen_output->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
	if (FAILED(hr)) {
		terminal_fill("Error: QueryInterface failed.\n", 12);
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

	std::cout << "Desktop duplication device created succesfully " << "\n";

	//texture description
	DXGI_OUTDUPL_DESC desktop_duplicate_desc;
	desktop_duplication->GetDesc(&desktop_duplicate_desc);
	ZeroMemory(&texture_desc, sizeof(texture_desc));
	texture_desc.Width = desktop_duplicate_desc.ModeDesc.Width;
	texture_desc.Height = desktop_duplicate_desc.ModeDesc.Height;
	texture_desc.MipLevels = 1;
	texture_desc.ArraySize = 1;
	texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                                                  		 // This is the default data when using desktop duplication, see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404611(v=vs.85).aspx
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Usage = D3D11_USAGE_STAGING /*D3D11_USAGE_DYNAMIC*/;					 					 // comments: this would lead to a GPU accessible texture only. --> TODO
	texture_desc.BindFlags = 0 /*D3D11_BIND_SHADER_RESOURCE*/;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE /*D3D11_CPU_ACCESS_WRITE*/; 
	texture_desc.MiscFlags = 0;
	std::cout << "Capturing a " << texture_desc.Width << " x " << texture_desc.Height << " monitor.\n";

	if (!check_cpu_access_texture(frame_texture)) {		
		terminal_fill("The used texture is not accessabil by the cpu. ", 12);
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
	if (((int)sleepTimerMs - 10) > 1)
		Sleep(sleepTimerMs - 10);
	// this value is specific for my system 
	// just give it some extra ms

    //get accumulated frames
	hr = desktop_duplication->AcquireNextFrame(5, &frame_info, &frame); // win desktop duplication apis' core function
	if (hr == DXGI_ERROR_INVALID_CALL) {
		return false;
	} if (hr == E_INVALIDARG) {
		return false;
	} if (frame_info.AccumulatedFrames == 0) {
		return false;
	} if (FAILED(hr) && hr != DXGI_ERROR_WAIT_TIMEOUT) {				// ^= (hr == DXGI_ERROR_ACCESS_LOST)
		desktop_duplication->ReleaseFrame();
		desktop_duplication.Release();
		//device.Release();
		//context.Release();
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

	//making it CPU accessible
	context->CopyResource(frame_texture, frame_texture_ori);

	/**
	 * best way to do it would be:
	 *  - capture in a 2DTexture in gpu ram, "/alternative params/"
	 *  - obtain a shaderstructure, apply a mipmap-chain with (custom) filters (covering the conditions of reject_sub_pixel()).
	 *  - memcpy() mipmaps to cpu access structure.
	 * but this way works just fine:
	 */
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
bool reject_sub_pixel(Pixel& curr_pixel) {
	if (min_brightness_per_pixel == 0 && min_saturation_per_pixel == 0)
		return false;
	else
		return ( ((curr_pixel.b < min_brightness_per_pixel) && (curr_pixel.g < min_brightness_per_pixel) && (curr_pixel.r < min_brightness_per_pixel)) || ((abs(curr_pixel.r - curr_pixel.g) < min_saturation_per_pixel) && (abs(curr_pixel.g - curr_pixel.b) < min_saturation_per_pixel)));
}

/**
 * @brief retrieve pixel data and calculate the mean of the latest frame directly
 * @param mapped_subresource
 * @return new_pixel mean b,g,r,a values of the current frame
 */
Pixel retrieve_pixel(D3D11_MAPPED_SUBRESOURCE& mapped_subresource) {

	const uint16_t height = texture_desc.Height;
	const uint16_t width = texture_desc.Width;

	// point to bytes/values of pixel data 
	uint8_t* pixel_array_source = static_cast<uint8_t*>(mapped_subresource.pData);

	Pixel curr_pixel;
	Pixel accum_pixel;
	Pixel mean_pixel;

	accum_pixel = { 0, 0, 0 };
	int pixel_amount = 0;
	for (UINT row = 0; row < height; row = row + 3) {							    // +2 instead of ++ drops half the resolution
		UINT row_start = row * mapped_subresource.RowPitch / 4;
		for (UINT col = 0; col < width; col = col + 2) {						    // col + quality_loss 

			curr_pixel.b = pixel_array_source[row_start + col * 4 + 0];             // first byte = b, according to "DXGI_FORMAT_B8G8R8A8_UNORM"
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

	if (pixel_amount == 0)															// avert division by zero 
		mean_pixel = mean_color_old;												// ..mh nothing found, let's send old frame and fade once more..
		//return mean_pixel = { 0, 0, 0 };											// send_data(): does nothing, if 'black'
		//return mean_pixel = {10, 0, 30};											// Default lila for dark scenes

	mean_pixel = {    //gamma adjustment
		accum_pixel.b == 0 ? 0 : (accum_pixel.b / pixel_amount + 1/*+ (accum_pixel.b % pixel_amount != 0)*/),		// commented additions: integer ceiling
		accum_pixel.g == 0 ? 0 : (accum_pixel.g / pixel_amount + 1/*+ (accum_pixel.g % pixel_amount != 0)*/),		
		accum_pixel.r == 0 ? 0 : (accum_pixel.r / pixel_amount + 1/*+ (accum_pixel.r % pixel_amount != 0)*/),
	};

	return mean_pixel;
}

/**
 * @brief fades the old color with the newly obtained one, to get not that drastically changing lights.
 * @param mean_pixel_new the new average color
 * @return mean_pixel_fade the faded color
 */
Pixel fade(Pixel& pixel) {
	Pixel pixel_faded = {
		(pixel.b * (256 - fade_val) + mean_color_old.b * fade_val) >> 8,
		(pixel.g * (256 - fade_val) + mean_color_old.g * fade_val) >> 8,
		(pixel.r * (256 - fade_val) + mean_color_old.r * fade_val) >> 8
	};

	return pixel_faded;
}

/**
 * @brief grabs the gamma corrected values for each color out of ::gamma8[]
 * @param pixel 
 * @return Pixel after gamma_correction 
 */
Pixel gamma_correction(Pixel& pixel){
	Pixel pixel_gamma_corrected = {
		gamma8[pixel.b == 0 ? 0 : pixel.b - (pixel.b / 3)],		// blue is a bit too intense with WS2812b ICs on 5050LEDs
		gamma8[pixel.g == 0 ? 0 : pixel.g - (pixel.g / 10)],	// and green is intense for the eyes 
		gamma8[pixel.r - 1] + 1									// always a little red
	};

	return pixel_gamma_corrected;
}

// true gamme correction:
// TODO
#if 0
std::vector<std::vector<uint8_t>> setup_gamma() {
	std::vector<std::vector<uint8_t>> gamma(256, std::vector<uint8_t>(3, 0));
	// Pre-compute gamma correction table for LED brightness levels:
	for (int i = 0; i < 256; ++i) {
		float f = pow((float)i / 255., 2.8);
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
	terminal_fill("\nTrying to connect with the LED controller\n\t...\n");
 
	SP = new Serial(serial_port);				// try my own default USB-port first
	if (SP->IsConnected()) {
		terminal_fill("Connection established! Let in the light!\n");
		return SP->IsConnected();
	}
	
	for (int i = 0; i < 10; ++i) {				// now every other Port from 0 to 9
		std::string s = "COM";
		s.push_back((char)(i + 48));
		const char* c = s.c_str();
		SP = new Serial(c);
		if (SP->IsConnected()) {
			terminal_fill("Connection established! Let in the light!\n");
			break;
		}
	}
	return SP->IsConnected();
}

/**
 * @brief send byte array to the micro controller. The first two bytes are the signature bytes. RGB8 gets attached.
 * @param pixel, the new color values to be sent.
 * @return true, if send_data() worked or frame was 'black'.
 */
bool send_data(Pixel& pixel) {
	if (pixel.r == 0 && pixel.g == 0 && pixel.b == 0)
		return true;
	uint8_t buffer[5] = { 'm', 'o', (uint8_t)pixel.r, (uint8_t)pixel.g, (uint8_t)pixel.b };
	//Sleep(1);

	return SP->WriteData(buffer, 5 /*sizeof(buffer)*/);
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
		Sleep(4500);
		create_and_get_device(chosen_output_num);		 	// not finished... how do i pass a specific device+monitor to CreateDevice() for different grakas and rotations?
		return true;
	}
	if (!connection_setup())
		return false;
	Sleep(1000);
	create_and_get_device(chosen_output_num);

	terminal_fill("I will run a test for 50 seconds, now.\n", 14);
	UINT user_fps = fps;
	std::vector<uint32_t> fps_vec = { user_fps, 25, 30, 60, 0 };
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
			mean_color_old = mean_color_new;
			if (!get_frame())
				continue;
			mean_color_new = retrieve_pixel(mapped_subresource);
			mean_color_new = fade(mean_color_new);
			Pixel mean_color_gamma_corrected = gamma_correction(mean_color_new);
			if (!send_data(mean_color_gamma_corrected)) {									// send data to micro controller
				terminal_fill("Sending data to the micro controller failed!\n\tTrying to reconnect...\n", 12);
				Sleep(5000);
				connection_setup();
				Sleep(5000);
			}
			++get_frame_call;
		}
		terminal_fill("I looped exactly " + std::to_string(get_frame_call) + " times.\n", 14);
		terminal_fill("I captured: ~" + std::to_string(get_frame_call / 10) + " fps.\n", 14);
	}
	set_sleepTimerMs(user_fps); 		// reset

	return true;
}

bool configuration() {
	set_sleepTimerMs(fps);
	std::string configurate = "n";
	terminal_fill("\nWould you like to configure the settings? (y/n):\n", 11);
	std::cin.clear();
	std::cin >> configurate;
	if (configurate == "y" || configurate == "Y") {
		terminal_fill("\nWhich framerate would you like to capture?\n\tType your (positive) number, or '0' to catch 'em all:\t\tDefault: 35 \n", 11);
		std::cin.clear();
		std::cin >> fps;
		set_sleepTimerMs(fps);
		std::cin.clear();

		int sat, bright = 0;
		terminal_fill("Insert min. saturation level of the pixel for the analyzation (0 - 255)):\t\tDefault: 18\n", 11);
		std::cin.clear();
		std::cin >> sat;
		min_saturation_per_pixel = sat;

		terminal_fill("Insert min. brightness level of the pixel for the analyzation (0 - 255)):\t\tDefault: 50\n", 11);
		std::cin.clear();
		std::cin >> bright;
		min_brightness_per_pixel = bright;

		terminal_fill("Insert fading factor from one frame to another (0 - 255):\t\tDefault: 110\n", 11);
		std::cin.clear();
		std::cin >> fade_val;
	}
	terminal_fill("\n--- Proceeding with following settings: ---\n");
	std::cout << "\tMin.Brightness per Pixel: " << (int)min_brightness_per_pixel << "\n";
	std::cout << "\tMin. Saturation per Pixel: " << (int)min_saturation_per_pixel << "\n";
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

	terminal_fill("\n--- Starting continues analyzation ---\n\n");

	while (true) {
		mean_color_old = mean_color_new;						// used in 'fade()'
		if (!get_frame()) 										// try no if-condition here for smoother lights when less than 24fps, but adjust send_data with a 1m sleep..
			continue;
		mean_color_new = retrieve_pixel(mapped_subresource);
		mean_color_new = fade(mean_color_new);
		Pixel mean_color_gamma_corrected = gamma_correction(mean_color_new);
		if (!send_data(mean_color_gamma_corrected)) {		// send data to micro controller
			terminal_fill("Error: Sending data to the micro controller failed!\n\tTrying to reconnect...\n", 12);
			Sleep(5000);
			connection_setup();
			Sleep(5000);
		}
		// prints:
		if (mapped_frames_counter % (fps + 1) == 0)
			std::cout << "new_avg_pixel: " << mean_color_gamma_corrected.r << "r, " << mean_color_gamma_corrected.g << "g, " << mean_color_gamma_corrected.b << "b   \r";
	}

	std::cin.clear();
	std::cout << "Press 'Enter' to end! \n";
	std::cin.ignore();

	// oupsie:
	// TODO: Clean Up !
	return 0;
}