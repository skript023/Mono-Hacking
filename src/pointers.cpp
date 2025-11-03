#include "common.hpp"
#include "logger.hpp"
#include "pointers.hpp"

namespace big
{
	pointers::pointers() : main_batch("pointer_cache")
	{
		if (!this->get_swapchain())
			LOG(WARNING) << "Failed get swapchain";
		
		
		main_batch.add("Return Address", "FF 23", [this](memory::handle ptr)
		{
			m_return_address = ptr.as<void*>();
		});

		main_batch.run(memory::module(nullptr));

		this->m_hwnd = FindWindow("UnrealWindow", "ScarletNexus  ");
		if (!this->m_hwnd)
			throw std::runtime_error("Failed to find the game's window.");

		g_pointers = this;
	}

	pointers::~pointers()
	{
		g_pointers = nullptr;
	}

	void pointers::update()
	{
		main_batch.update();
	}

	bool pointers::get_swapchain()
	{
		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.hIcon = NULL;
		windowClass.hCursor = NULL;
		windowClass.hbrBackground = NULL;
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = "Kiero";
		windowClass.hIconSm = NULL;

		::RegisterClassEx(&windowClass);

		this->m_window = ::CreateWindow(windowClass.lpszClassName, "Kiero DirectX Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

		if (this->m_window == NULL)
		{
			return false;
		}

		HMODULE d3d11 = ::GetModuleHandle("d3d11.dll");
		if (d3d11 == NULL)
		{
			::DestroyWindow(this->m_window);
			::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
			return false;
		}

		void* create_device_and_swapchain = ::GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");
		if (create_device_and_swapchain == NULL)
		{
			::DestroyWindow(this->m_window);
			::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
			return false;
		}

		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1, };

		DXGI_RATIONAL refreshRate;
		refreshRate.Numerator = 60;
		refreshRate.Denominator = 1;

		DXGI_MODE_DESC bufferDesc;
		bufferDesc.Width = 100;
		bufferDesc.Height = 100;
		bufferDesc.RefreshRate = refreshRate;
		bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		DXGI_SAMPLE_DESC sampleDesc;
		sampleDesc.Count = 1;
		sampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		swapChainDesc.BufferDesc = bufferDesc;
		swapChainDesc.SampleDesc = sampleDesc;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.OutputWindow = this->m_window;
		swapChainDesc.Windowed = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT hr = ((functions::create_d3d11_device_and_swapchain_t)(create_device_and_swapchain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &this->m_swapchain, &this->m_d3d_device, &featureLevel, &this->m_d3d_context);
		
		if (FAILED(hr))
		{
			::DestroyWindow(this->m_window);
			::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
			return false;
		}

		::memcpy(this->m_swapchain_methods, *(void***)this->m_swapchain, sizeof(m_swapchain_methods));

		this->m_swapchain->Release();
		this->m_swapchain = nullptr;

		this->m_d3d_device->Release();
		this->m_d3d_device = nullptr;

		this->m_d3d_context->Release();
		this->m_d3d_context = nullptr;

		::DestroyWindow(this->m_window);
		::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

		return true;
	}
}