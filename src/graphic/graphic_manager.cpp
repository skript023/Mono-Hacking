#include "graphic_manager.hpp"
#include "hooking.hpp"

#define GRAPHIC_ARRAYSIZE(arr) ((size_t)(sizeof(arr) / sizeof(arr[0])))

namespace big
{
    graphic_manager& graphic_manager::get_instance()
    {
        static graphic_manager instance;
        return instance;
    }
    std::string graphic_manager::get_render_type_name()
    {
        auto hash = graphic_manager::get_instance().m_render_type;

        switch (hash)
        {
        case eGraphicsAPI::none:
            return "No Render Type";
        case eGraphicsAPI::directx9:
            return "DirectX 9";
        case eGraphicsAPI::directx10:
            return "DirectX 10";
        case eGraphicsAPI::directx11:
            return "DirectX 11";
        case eGraphicsAPI::directx12:
            return "DirectX 12";
		case eGraphicsAPI::opengl:
			return "OpenGL";
		case eGraphicsAPI::vulkan:
			return "Vulkan";
        }

        return "Unknown Type";
    }
    void* graphic_manager::get_method_table_impl(int index)
    {
        if (!m_swapchain_methods.empty())
        {
            auto address = (void*)m_swapchain_methods[index];

            LOG(INFO) << "Address of index : " << index << " is " << address;
            return address;
        }

        return nullptr;
    }
    eGraphicsAPI graphic_manager::detect_graphics_api()
    {
        if (GetModuleHandleA("d3d9.dll"))
        {
            return eGraphicsAPI::directx9;
        }
        else if (GetModuleHandleA("d3d10.dll"))
        {
            return eGraphicsAPI::directx10;
        }
        else if (GetModuleHandleA("d3d11.dll"))
        {
            return eGraphicsAPI::directx11;
        }
        else if (GetModuleHandleA("d3d12.dll"))
        {
            return eGraphicsAPI::directx12;
        }
        else if (GetModuleHandleA("opengl32.dll"))
        {
            return eGraphicsAPI::opengl;
        }
        else if (GetModuleHandleA("vulkan-1.dll"))
        {
            return eGraphicsAPI::vulkan;
        }
        return eGraphicsAPI::none;
    }
    eInitializationStatus graphic_manager::create_dummy_device(eGraphicsAPI renderType)
	{
        WNDCLASSEX window_class;
        window_class.cbSize = sizeof(WNDCLASSEX);
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = DefWindowProc;
        window_class.cbClsExtra = 0;
        window_class.cbWndExtra = 0;
        window_class.hInstance = GetModuleHandle(NULL);
        window_class.hIcon = NULL;
        window_class.hCursor = NULL;
        window_class.hbrBackground = NULL;
        window_class.lpszMenuName = NULL;
        window_class.lpszClassName = "Ellohim";
        window_class.hIconSm = NULL;

        ::RegisterClassEx(&window_class);

        HWND window = ::CreateWindow(window_class.lpszClassName, "Ellohim Dummy Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, window_class.hInstance, NULL);

        if (window == NULL)
        {
            return eInitializationStatus::FAILED;
        }

        if (renderType == eGraphicsAPI::directx9)
        {
            HMODULE d3d9_module = ::GetModuleHandle("d3d9.dll");
            if (d3d9_module == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* Direct3DCreate9 = ::GetProcAddress(d3d9_module, "Direct3DCreate9");
            if (Direct3DCreate9 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            auto create_d3d9 = ((LPDIRECT3D9(__stdcall*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION);
            if (create_d3d9 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3DDISPLAYMODE DisplayMode;
            if (FAILED(create_d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &DisplayMode)))
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3DPRESENT_PARAMETERS d3d9_params;
            d3d9_params.BackBufferWidth = 0;
            d3d9_params.BackBufferHeight = 0;
            d3d9_params.BackBufferFormat = DisplayMode.Format;
            d3d9_params.BackBufferCount = 0;
            d3d9_params.MultiSampleType = D3DMULTISAMPLE_NONE;
            d3d9_params.MultiSampleQuality = NULL;
            d3d9_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
            d3d9_params.hDeviceWindow = window;
            d3d9_params.Windowed = 1;
            d3d9_params.EnableAutoDepthStencil = 0;
            d3d9_params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
            d3d9_params.Flags = NULL;
            d3d9_params.FullScreen_RefreshRateInHz = 0;
            d3d9_params.PresentationInterval = 0;

            LPDIRECT3DDEVICE9 d3d9_device;
            if (create_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &d3d9_params, &d3d9_device) < 0)
            {
                create_d3d9->Release();
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

   //         m_swapchain_methods = (uintptr_t*)calloc(120, sizeof(uintptr_t));
			//memcpy(m_swapchain_methods, *(uintptr_t**)d3d9_device, sizeof(uintptr_t));
			m_swapchain_methods.clear();
			m_swapchain_methods.reserve(120);

			m_swapchain_methods.append_range(vtable_view(d3d9_device, 1));

            create_d3d9->Release();
            create_d3d9 = nullptr;
            d3d9_device->Release();
            d3d9_device = nullptr;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = eGraphicsAPI::directx9;

            return eInitializationStatus::SUCCESS;
        }
        else if (renderType == eGraphicsAPI::directx11)
        {
            HMODULE d3d11 = ::GetModuleHandle("d3d11.dll");
            if (d3d11 == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* create_device_and_swapchain = ::GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain");
            if (create_device_and_swapchain == NULL)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
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
            swapChainDesc.OutputWindow = window;
            swapChainDesc.Windowed = 1;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            IDXGISwapChain* d3d11_swapchain = nullptr;
            ID3D11Device* d3d11_device = nullptr;
            ID3D11DeviceContext* d3d11_context = nullptr;

            HRESULT hr = ((create_d3d11_device_and_swapchain_t)(create_device_and_swapchain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &d3d11_swapchain, &d3d11_device, &featureLevel, &d3d11_context);

            if (FAILED(hr))
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

   //         m_swapchain_methods = (uintptr_t*)calloc(205, sizeof(uintptr_t));
			//memcpy(this->m_swapchain_methods, *(uintptr_t**)d3d11_swapchain, 18 * sizeof(uintptr_t));
			//memcpy(this->m_swapchain_methods + 18, *(uintptr_t**)d3d11_device, 43 * sizeof(uintptr_t));
			//memcpy(this->m_swapchain_methods + 18 + 43, *(uintptr_t**)d3d11_context, 144 * sizeof(uintptr_t));

			m_swapchain_methods.clear();
			m_swapchain_methods.reserve(205);

			m_swapchain_methods.append_range(vtable_view(d3d11_swapchain, 18));
			m_swapchain_methods.append_range(vtable_view(d3d11_device, 43));
			m_swapchain_methods.append_range(vtable_view(d3d11_context, 144));

            d3d11_swapchain->Release();
            d3d11_swapchain = nullptr;

            d3d11_device->Release();
            d3d11_device = nullptr;

            d3d11_context->Release();
            d3d11_context = nullptr;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = eGraphicsAPI::directx11;

            return eInitializationStatus::SUCCESS;
        }
        else if (renderType == eGraphicsAPI::directx12)
        {
            HMODULE dxgi_module;
            HMODULE d3d12_module;
            if ((dxgi_module = ::GetModuleHandle("dxgi.dll")) == NULL || (d3d12_module = ::GetModuleHandle("d3d12.dll")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                LOG(FATAL) << "Failed get dx12 & dxgi module";

                return eInitializationStatus::MODULE_NOT_FOUND_ERROR;
            }

            void* CreateDXGIFactory;
            if ((CreateDXGIFactory = GetProcAddress(dxgi_module, "CreateDXGIFactory")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            IDXGIFactory* factory;
            if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            IDXGIAdapter* adapter;
            if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            void* D3D12CreateDevice;
            if ((D3D12CreateDevice = ::GetProcAddress(d3d12_module, "D3D12CreateDevice")) == NULL)
            {
                DestroyWindow(window);
                UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12Device* device;
            if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            D3D12_COMMAND_QUEUE_DESC queueDesc;
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Priority = 0;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 0;

            ID3D12CommandQueue* commandQueue;
            if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12CommandAllocator* commandAllocator;
            if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocator) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

            ID3D12GraphicsCommandList* commandList;
            if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::FAILED;
            }

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

            DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
            swapChainDesc.BufferDesc = bufferDesc;
            swapChainDesc.SampleDesc = sampleDesc;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = 2;
            swapChainDesc.OutputWindow = window;
            swapChainDesc.Windowed = 1;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            IDXGISwapChain* swapChain;
            if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0)
            {
                ::DestroyWindow(window);
                ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

                return eInitializationStatus::CREATE_DUMMY_DEVICE_ERROR;
            }

   //         m_swapchain_methods = (uintptr_t*)calloc(150, sizeof(uintptr_t));
			//::memcpy(m_swapchain_methods, *(uintptr_t**)device, 44 * sizeof(uintptr_t));
			//::memcpy(m_swapchain_methods + 44, *(uintptr_t**)commandQueue, 19 * sizeof(uintptr_t));
			//::memcpy(m_swapchain_methods + 44 + 19, *(uintptr_t**)commandAllocator, 9 * sizeof(uintptr_t));
			//::memcpy(m_swapchain_methods + 44 + 19 + 9, *(uintptr_t**)commandList, 60 * sizeof(uintptr_t));
			//::memcpy(m_swapchain_methods + 44 + 19 + 9 + 60, *(uintptr_t**)swapChain, 18 * sizeof(uintptr_t));

			m_swapchain_methods.clear();
			m_swapchain_methods.reserve(150);

			m_swapchain_methods.append_range(vtable_view(device, 44));
			m_swapchain_methods.append_range(vtable_view(commandQueue, 19));
			m_swapchain_methods.append_range(vtable_view(commandAllocator, 9));
			m_swapchain_methods.append_range(vtable_view(commandList, 60));
			m_swapchain_methods.append_range(vtable_view(swapChain, 18));

            device->Release();
            device = NULL;

            commandQueue->Release();
            commandQueue = NULL;

            commandAllocator->Release();
            commandAllocator = NULL;

            commandList->Release();
            commandList = NULL;

            swapChain->Release();
            swapChain = NULL;

            ::DestroyWindow(window);
            ::UnregisterClass(window_class.lpszClassName, window_class.hInstance);

            m_render_type = eGraphicsAPI::directx12;

            return eInitializationStatus::SUCCESS;
        }
		else if (renderType == eGraphicsAPI::opengl)
        {
			HMODULE libOpenGL32;
			if ((libOpenGL32 = ::GetModuleHandle("opengl32.dll")) == NULL)
			{
				return eInitializationStatus::FAILED;
			}

			const char* const methodsNames[] = {
			    "glAccum",
			    "glAlphaFunc",
			    "glAreTexturesResident",
			    "glArrayElement",
			    "glBegin",
			    "glBindTexture",
			    "glBitmap",
			    "glBlendFunc",
			    "glCallList",
			    "glCallLists",
			    "glClear",
			    "glClearAccum",
			    "glClearColor",
			    "glClearDepth",
			    "glClearIndex",
			    "glClearStencil",
			    "glClipPlane",
			    "glColor3b",
			    "glColor3bv",
			    "glColor3d",
			    "glColor3dv",
			    "glColor3f",
			    "glColor3fv",
			    "glColor3i",
			    "glColor3iv",
			    "glColor3s",
			    "glColor3sv",
			    "glColor3ub",
			    "glColor3ubv",
			    "glColor3ui",
			    "glColor3uiv",
			    "glColor3us",
			    "glColor3usv",
			    "glColor4b",
			    "glColor4bv",
			    "glColor4d",
			    "glColor4dv",
			    "glColor4f",
			    "glColor4fv",
			    "glColor4i",
			    "glColor4iv",
			    "glColor4s",
			    "glColor4sv",
			    "glColor4ub",
			    "glColor4ubv",
			    "glColor4ui",
			    "glColor4uiv",
			    "glColor4us",
			    "glColor4usv",
			    "glColorMask",
			    "glColorMaterial",
			    "glColorPointer",
			    "glCopyPixels",
			    "glCopyTexImage1D",
			    "glCopyTexImage2D",
			    "glCopyTexSubImage1D",
			    "glCopyTexSubImage2D",
			    "glCullFaceglCullFace",
			    "glDeleteLists",
			    "glDeleteTextures",
			    "glDepthFunc",
			    "glDepthMask",
			    "glDepthRange",
			    "glDisable",
			    "glDisableClientState",
			    "glDrawArrays",
			    "glDrawBuffer",
			    "glDrawElements",
			    "glDrawPixels",
			    "glEdgeFlag",
			    "glEdgeFlagPointer",
			    "glEdgeFlagv",
			    "glEnable",
			    "glEnableClientState",
			    "glEnd",
			    "glEndList",
			    "glEvalCoord1d",
			    "glEvalCoord1dv",
			    "glEvalCoord1f",
			    "glEvalCoord1fv",
			    "glEvalCoord2d",
			    "glEvalCoord2dv",
			    "glEvalCoord2f",
			    "glEvalCoord2fv",
			    "glEvalMesh1",
			    "glEvalMesh2",
			    "glEvalPoint1",
			    "glEvalPoint2",
			    "glFeedbackBuffer",
			    "glFinish",
			    "glFlush",
			    "glFogf",
			    "glFogfv",
			    "glFogi",
			    "glFogiv",
			    "glFrontFace",
			    "glFrustum",
			    "glGenLists",
			    "glGenTextures",
			    "glGetBooleanv",
			    "glGetClipPlane",
			    "glGetDoublev",
			    "glGetError",
			    "glGetFloatv",
			    "glGetIntegerv",
			    "glGetLightfv",
			    "glGetLightiv",
			    "glGetMapdv",
			    "glGetMapfv",
			    "glGetMapiv",
			    "glGetMaterialfv",
			    "glGetMaterialiv",
			    "glGetPixelMapfv",
			    "glGetPixelMapuiv",
			    "glGetPixelMapusv",
			    "glGetPointerv",
			    "glGetPolygonStipple",
			    "glGetString",
			    "glGetTexEnvfv",
			    "glGetTexEnviv",
			    "glGetTexGendv",
			    "glGetTexGenfv",
			    "glGetTexGeniv",
			    "glGetTexImage",
			    "glGetTexLevelParameterfv",
			    "glGetTexLevelParameteriv",
			    "glGetTexParameterfv",
			    "glGetTexParameteriv",
			    "glHint",
			    "glIndexMask",
			    "glIndexPointer",
			    "glIndexd",
			    "glIndexdv",
			    "glIndexf",
			    "glIndexfv",
			    "glIndexi",
			    "glIndexiv",
			    "glIndexs",
			    "glIndexsv",
			    "glIndexub",
			    "glIndexubv",
			    "glInitNames",
			    "glInterleavedArrays",
			    "glIsEnabled",
			    "glIsList",
			    "glIsTexture",
			    "glLightModelf",
			    "glLightModelfv",
			    "glLightModeli",
			    "glLightModeliv",
			    "glLightf",
			    "glLightfv",
			    "glLighti",
			    "glLightiv",
			    "glLineStipple",
			    "glLineWidth",
			    "glListBase",
			    "glLoadIdentity",
			    "glLoadMatrixd",
			    "glLoadMatrixf",
			    "glLoadName",
			    "glLogicOp",
			    "glMap1d",
			    "glMap1f",
			    "glMap2d",
			    "glMap2f",
			    "glMapGrid1d",
			    "glMapGrid1f",
			    "glMapGrid2d",
			    "glMapGrid2f",
			    "glMaterialf",
			    "glMaterialfv",
			    "glMateriali",
			    "glMaterialiv",
			    "glMatrixMode",
			    "glMultMatrixd",
			    "glMultMatrixf",
			    "glNewList",
			    "glNormal3b",
			    "glNormal3bv",
			    "glNormal3d",
			    "glNormal3dv",
			    "glNormal3f",
			    "glNormal3fv",
			    "glNormal3i",
			    "glNormal3iv",
			    "glNormal3s",
			    "glNormal3sv",
			    "glNormalPointer",
			    "glOrtho",
			    "glPassThrough",
			    "glPixelMapfv",
			    "glPixelMapuiv",
			    "glPixelMapusv",
			    "glPixelStoref",
			    "glPixelStorei",
			    "glPixelTransferf",
			    "glPixelTransferi",
			    "glPixelZoom",
			    "glPointSize",
			    "glPolygonMode",
			    "glPolygonOffset",
			    "glPolygonStipple",
			    "glPopAttrib",
			    "glPopClientAttrib",
			    "glPopMatrix",
			    "glPopName",
			    "glPrioritizeTextures",
			    "glPushAttrib",
			    "glPushClientAttrib",
			    "glPushMatrix",
			    "glPushName",
			    "glRasterPos2d",
			    "glRasterPos2dv",
			    "glRasterPos2f",
			    "glRasterPos2fv",
			    "glRasterPos2i",
			    "glRasterPos2iv",
			    "glRasterPos2s",
			    "glRasterPos2sv",
			    "glRasterPos3d",
			    "glRasterPos3dv",
			    "glRasterPos3f",
			    "glRasterPos3fv",
			    "glRasterPos3i",
			    "glRasterPos3iv",
			    "glRasterPos3s",
			    "glRasterPos3sv",
			    "glRasterPos4d",
			    "glRasterPos4dv",
			    "glRasterPos4f",
			    "glRasterPos4fv",
			    "glRasterPos4i",
			    "glRasterPos4iv",
			    "glRasterPos4s",
			    "glRasterPos4sv",
			    "glReadBuffer",
			    "glReadPixels",
			    "glRectd",
			    "glRectdv",
			    "glRectf",
			    "glRectfv",
			    "glRecti",
			    "glRectiv",
			    "glRects",
			    "glRectsv",
			    "glRenderMode",
			    "glRotated",
			    "glRotatef",
			    "glScaled",
			    "glScalef",
			    "glScissor",
			    "glSelectBuffer",
			    "glShadeModel",
			    "glStencilFunc",
			    "glStencilMask",
			    "glStencilOp",
			    "glTexCoord1d",
			    "glTexCoord1dv",
			    "glTexCoord1f",
			    "glTexCoord1fv",
			    "glTexCoord1i",
			    "glTexCoord1iv",
			    "glTexCoord1s",
			    "glTexCoord1sv",
			    "glTexCoord2d",
			    "glTexCoord2dv",
			    "glTexCoord2f",
			    "glTexCoord2fv",
			    "glTexCoord2i",
			    "glTexCoord2iv",
			    "glTexCoord2s",
			    "glTexCoord2sv",
			    "glTexCoord3d",
			    "glTexCoord3dv",
			    "glTexCoord3f",
			    "glTexCoord3fv",
			    "glTexCoord3i",
			    "glTexCoord3iv",
			    "glTexCoord3s",
			    "glTexCoord3sv",
			    "glTexCoord4d",
			    "glTexCoord4dv",
			    "glTexCoord4f",
			    "glTexCoord4fv",
			    "glTexCoord4i",
			    "glTexCoord4iv",
			    "glTexCoord4s",
			    "glTexCoord4sv",
			    "glTexCoordPointer",
			    "glTexEnvf",
			    "glTexEnvfv",
			    "glTexEnvi",
			    "glTexEnviv",
			    "glTexGend",
			    "glTexGendv",
			    "glTexGenf",
			    "glTexGenfv",
			    "glTexGeni",
			    "glTexGeniv",
			    "glTexImage1D",
			    "glTexImage2D",
			    "glTexParameterf",
			    "glTexParameterfv",
			    "glTexParameteri",
			    "glTexParameteriv",
			    "glTexSubImage1D",
			    "glTexSubImage2D",
			    "glTranslated",
			    "glTranslatef",
			    "glVertex2d",
			    "glVertex2dv",
			    "glVertex2f",
			    "glVertex2fv",
			    "glVertex2i",
			    "glVertex2iv",
			    "glVertex2s",
			    "glVertex2sv",
			    "glVertex3d",
			    "glVertex3dv",
			    "glVertex3f",
			    "glVertex3fv",
			    "glVertex3i",
			    "glVertex3iv",
			    "glVertex3s",
			    "glVertex3sv",
			    "glVertex4d",
			    "glVertex4dv",
			    "glVertex4f",
			    "glVertex4fv",
			    "glVertex4i",
			    "glVertex4iv",
			    "glVertex4s",
			    "glVertex4sv",
			    "glVertexPointer",
			    "glViewport"
			};

			size_t size = GRAPHIC_ARRAYSIZE(methodsNames);

			m_swapchain_methods.clear();
			m_swapchain_methods.reserve(size);

			for (int i = 0; i < size; i++)
			{
				FARPROC proc = ::GetProcAddress(libOpenGL32, methodsNames[i]);

				if (proc)
				{
					m_swapchain_methods[i] = reinterpret_cast<uintptr_t>(proc);
				}
			}
        }

        return eInitializationStatus::FAILED;
	}
}