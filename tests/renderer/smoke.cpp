#include "renderer.hpp"
#include "render_dx11.hpp"
#include "render_vulkan.hpp"
#include <wrl/client.h>
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <backends/imgui_impl_dx11.h>
using Microsoft::WRL::ComPtr;
using namespace big;
static void require(bool ok, const char* message)
{
	if (!ok)
		throw std::runtime_error(message);
}
static void checked(VkResult result)
{
	require(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, ("Vulkan result " + std::to_string(result)).c_str());
}
static void test_icon_cache_textures(render_backend& backend)
{
	const unsigned char pixel[] = {255, 255, 255, 255};
	for (int pass = 0; pass < 3; ++pass)
	{
		std::vector<ImTextureID> icons;
		for (int i = 0; i < 128; ++i)
		{
			auto id = backend.upload_rgba(pixel, 1, 1);
			require(id != 0, "Icon cache texture allocation failed");
			icons.push_back(id);
		}
		for (auto id : icons)
		{
			backend.release_texture(id);
			backend.release_texture(id);
		}
		backend.release_texture(0);
	}
}
void big::renderer::finish_init()
{
	const unsigned char green[16] = {0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255};
	render_vulkan upload;
	test_icon_cache_textures(upload);
	texture = upload.upload_rgba(green, 2, 2);
	require(texture != 0, "Vulkan texture upload failed");
	m_init = true;
}
static void test_dx11(HWND window, renderer& ui)
{
	DXGI_SWAP_CHAIN_DESC description{};
	description.BufferDesc.Width = description.BufferDesc.Height = 128;
	description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	description.SampleDesc.Count = 1;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.BufferCount = 2;
	description.OutputWindow = window;
	description.Windowed = TRUE;
	description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGISwapChain> swapchain;
	require(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &description, &swapchain, &device, nullptr, &context)), "DX11 device creation failed");
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr;
	ImGui_ImplWin32_Init(window);
	render_dx11 backend;
	require(backend.init(swapchain.Get()), "DX11 backend init failed");
	test_icon_cache_textures(backend);
	const unsigned char green[16] = {0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255};
	ui.texture = backend.upload_rgba(green, 2, 2);
	require(ui.texture != 0, "DX11 texture upload failed");
	for (int iteration = 0; iteration < 3; ++iteration)
	{
		ComPtr<ID3D11Texture2D> image;
		ComPtr<ID3D11RenderTargetView> target;
		require(SUCCEEDED(swapchain->GetBuffer(0, IID_PPV_ARGS(&image))), "DX11 GetBuffer failed");
		require(SUCCEEDED(device->CreateRenderTargetView(image.Get(), nullptr, &target)), "DX11 target creation failed");
		const float blue[] = {0, 0, 1, 1};
		context->ClearRenderTargetView(target.Get(), blue);
		auto original = target.Get();
		context->OMSetRenderTargets(1, &original, nullptr);
		backend.present();
		ComPtr<ID3D11RenderTargetView> restored;
		context->OMGetRenderTargets(1, &restored, nullptr);
		require(restored.Get() == target.Get(), "DX11 render target was not restored");
		D3D11_TEXTURE2D_DESC desc{};
		image->GetDesc(&desc);
		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ComPtr<ID3D11Texture2D> staging;
		require(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging)), "DX11 staging allocation failed");
		context->CopyResource(staging.Get(), image.Get());
		D3D11_MAPPED_SUBRESOURCE mapped{};
		require(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)), "DX11 readback failed");
		const auto pixel = [&](int x, int y) {
			return static_cast<unsigned char*>(mapped.pData) + y * mapped.RowPitch + x * 4;
		};
		require(pixel(12, 12)[0] > 240 && pixel(12, 12)[2] < 10, "DX11 overlay pixel incorrect");
		require(pixel(44, 12)[1] > 240, "DX11 texture pixel incorrect");
		require(pixel(100, 100)[2] > 240, "DX11 game background was overwritten");
		context->Unmap(staging.Get(), 0);
		backend.pre_reset();
		restored.Reset();
		target.Reset();
		image.Reset();
		staging.Reset();
		require(SUCCEEDED(swapchain->ResizeBuffers(2, 128, 128, DXGI_FORMAT_UNKNOWN, 0)), "DX11 resize failed");
		backend.post_reset(swapchain.Get());
	}
	backend.shutdown();
	backend.shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	std::cout << "DX11 PASS: pixels, textures, state restoration, resize, repeated shutdown\n";
}
static void test_vulkan(HWND window, renderer& ui, bool late = false, bool acquire2 = false)
{
	auto module = LoadLibraryW(L"vulkan-1.dll");
	require(module != nullptr, "Vulkan runtime unavailable");
	auto get = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(module, "vkGetInstanceProcAddr"));
	if (!late)
		render_vulkan::start_capture();
	auto create = reinterpret_cast<PFN_vkCreateInstance>(get(nullptr, "vkCreateInstance"));
	const char* extensions[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
	VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	application.apiVersion = VK_API_VERSION_1_1;
	VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	info.pApplicationInfo = &application;
	info.enabledExtensionCount = 2;
	info.ppEnabledExtensionNames = extensions;
	VkInstance instance{};
	checked(create(&info, nullptr, &instance));
#define INSTANCE(name)                                                     \
	auto name = reinterpret_cast<PFN_vk##name>(get(instance, "vk" #name)); \
	require(name != nullptr, "Missing vk" #name)
	INSTANCE(EnumeratePhysicalDevices);
	INSTANCE(GetPhysicalDeviceQueueFamilyProperties);
	INSTANCE(GetPhysicalDeviceProperties);
	INSTANCE(GetPhysicalDeviceSurfaceSupportKHR);
	INSTANCE(CreateWin32SurfaceKHR);
	INSTANCE(CreateDevice);
	INSTANCE(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	INSTANCE(GetPhysicalDeviceSurfaceFormatsKHR);
	INSTANCE(DestroySurfaceKHR);
	INSTANCE(DestroyInstance);
	INSTANCE(GetDeviceProcAddr);
	VkWin32SurfaceCreateInfoKHR surface_info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
	surface_info.hinstance = GetModuleHandleW(nullptr);
	surface_info.hwnd = window;
	VkSurfaceKHR surface{};
	checked(CreateWin32SurfaceKHR(instance, &surface_info, nullptr, &surface));
	uint32_t count = 0;
	checked(EnumeratePhysicalDevices(instance, &count, nullptr));
	require(count != 0, "No Vulkan GPU");
	std::vector<VkPhysicalDevice> physical(count);
	checked(EnumeratePhysicalDevices(instance, &count, physical.data()));
	VkPhysicalDevice gpu{};
	uint32_t family = 0;
	for (auto candidate : physical)
	{
		uint32_t size = 0;
		GetPhysicalDeviceQueueFamilyProperties(candidate, &size, nullptr);
		std::vector<VkQueueFamilyProperties> families(size);
		GetPhysicalDeviceQueueFamilyProperties(candidate, &size, families.data());
		for (uint32_t i = 0; i < size; ++i)
		{
			VkBool32 present = false;
			checked(GetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present));
			if (present && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				gpu = candidate;
				family = i;
				break;
			}
		}
		if (gpu)
			break;
	}
	require(gpu != VK_NULL_HANDLE, "No graphics/present queue");
	VkPhysicalDeviceProperties gpu_properties{};
	GetPhysicalDeviceProperties(gpu, &gpu_properties);
	float priority = 1;
	VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_info.queueFamilyIndex = family;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &priority;
	const char* swapchain_extension = "VK_KHR_swapchain";
	VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = 1;
	device_info.ppEnabledExtensionNames = &swapchain_extension;
	VkDevice device{};
	checked(CreateDevice(gpu, &device_info, nullptr, &device));
#define DEVICE(name)                                                                   \
	auto name = reinterpret_cast<PFN_vk##name>(GetDeviceProcAddr(device, "vk" #name)); \
	require(name != nullptr, "Missing vk" #name)
	DEVICE(GetDeviceQueue);
	DEVICE(CreateSwapchainKHR);
	DEVICE(DestroySwapchainKHR);
	DEVICE(GetSwapchainImagesKHR);
	DEVICE(CreateCommandPool);
	DEVICE(AllocateCommandBuffers);
	DEVICE(BeginCommandBuffer);
	DEVICE(EndCommandBuffer);
	DEVICE(ResetCommandBuffer);
	DEVICE(CmdPipelineBarrier);
	DEVICE(CmdClearColorImage);
	DEVICE(CreateSemaphore);
	DEVICE(DestroySemaphore);
	DEVICE(CreateFence);
	DEVICE(DestroyFence);
	DEVICE(AcquireNextImage2KHR);
	DEVICE(AcquireNextImageKHR);
	DEVICE(WaitForFences);
	DEVICE(ResetFences);
	DEVICE(QueueSubmit);
	DEVICE(QueuePresentKHR);
	DEVICE(DeviceWaitIdle);
	DEVICE(DestroyCommandPool);
	DEVICE(DestroyDevice);
	VkQueue queue{};
	GetDeviceQueue(device, family, 0, &queue);
	VkSurfaceCapabilitiesKHR capabilities{};
	checked(GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities));
	uint32_t format_count = 0;
	checked(GetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr));
	std::vector<VkSurfaceFormatKHR> formats(format_count);
	checked(GetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, formats.data()));
	VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = (std::max)(2u, capabilities.minImageCount);
	swapchain_info.imageFormat = formats[0].format;
	swapchain_info.imageColorSpace = formats[0].colorSpace;
	if (late)
	{
		bool found = false;
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				swapchain_info.imageFormat = format.format;
				swapchain_info.imageColorSpace = format.colorSpace;
				found = true;
				break;
			}
		}
		require(found, "Late attachment fixture requires BGRA8 UNORM");
	}
	swapchain_info.imageExtent = capabilities.currentExtent;
	if (swapchain_info.imageExtent.width == UINT32_MAX)
		swapchain_info.imageExtent = {128, 128};
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.preTransform = capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchain_info.clipped = true;
	VkSwapchainKHR swapchain{};
	checked(CreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain));
	VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	pool_info.queueFamilyIndex = family;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkCommandPool pool{};
	checked(CreateCommandPool(device, &pool_info, nullptr, &pool));
	VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	command_info.commandPool = pool;
	command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_info.commandBufferCount = 1;
	VkCommandBuffer command{};
	checked(AllocateCommandBuffers(device, &command_info, &command));
	VkSemaphoreCreateInfo sem_info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VkSemaphore ready[2]{};
	for (auto& semaphore : ready)
		checked(CreateSemaphore(device, &sem_info, nullptr, &semaphore));
	VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	VkFence acquire_fence{};
	checked(CreateFence(device, &fence_info, nullptr, &acquire_fence));
	for (int generation = 0; generation < 3; ++generation)
	{
		uint32_t image_count = 0;
		checked(GetSwapchainImagesKHR(device, swapchain, &image_count, nullptr));
		std::vector<VkImage> images(image_count);
		checked(GetSwapchainImagesKHR(device, swapchain, &image_count, images.data()));
		for (int frame = 0; frame < (generation == 0 ? 11 : (generation == 2 ? 10 : 8)); ++frame)
		{
			if (generation == 0 && frame == 3)
				render_vulkan::attach(gpu_properties.vendorID, gpu_properties.deviceID);
			if (generation == 2 && frame == 8)
			{
				render_vulkan::detach();
				render_vulkan::detach();
			}
			if (generation == 2 && frame == 9)
				render_vulkan::stop_capture();
			uint32_t index = 0;
			if (acquire2)
			{
				VkAcquireNextImageInfoKHR acquire_info{VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR};
				acquire_info.swapchain = swapchain;
				acquire_info.timeout = UINT64_MAX;
				acquire_info.fence = acquire_fence;
				acquire_info.deviceMask = 1;
				checked(AcquireNextImage2KHR(device, &acquire_info, &index));
			}
			else
				checked(AcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, acquire_fence, &index));
			checked(WaitForFences(device, 1, &acquire_fence, true, UINT64_MAX));
			checked(ResetFences(device, 1, &acquire_fence));
			checked(ResetCommandBuffer(command, 0));
			VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
			checked(BeginCommandBuffer(command, &begin));
			VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = images[index];
			barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			VkClearColorValue blue{{0, 0, 1, 1}};
			CmdClearColorImage(command, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &blue, 1, &barrier.subresourceRange);
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			checked(EndCommandBuffer(command));
			VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &command;
			submit.signalSemaphoreCount = frame % 3;
			submit.pSignalSemaphores = ready;
			checked(QueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
			VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
			present.waitSemaphoreCount = frame % 3;
			present.pWaitSemaphores = ready;
			present.swapchainCount = 1;
			present.pSwapchains = &swapchain;
			present.pImageIndices = &index;
			checked(QueuePresentKHR(queue, &present));
			checked(DeviceWaitIdle(device));
			if (generation == 0 && frame < 3)
				require(!ui.m_init && ui.draws == 0, "Overlay rendered before attachment");
			else if (generation == 2 && frame >= 8)
				require(!ui.m_init && ui.draws == 24, "Vulkan rendered after detach/unhook");
			else
				require(ui.m_init && ui.draws == (generation == 0 ? frame - 2 : generation * 8 + frame + 1), "Vulkan hook did not render");
		}
		if (generation != 2)
		{
			SetWindowPos(window, nullptr, 0, 0, 180 + generation * 20, 200 + generation * 20, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			checked(GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities));
			swapchain_info.imageExtent = capabilities.currentExtent;
			for (const auto& format : formats)
			{
				if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
				{
					swapchain_info.imageFormat = format.format;
					swapchain_info.imageColorSpace = format.colorSpace;
					break;
				}
			}
			swapchain_info.oldSwapchain = swapchain;
			VkSwapchainKHR replacement{};
			checked(CreateSwapchainKHR(device, &swapchain_info, nullptr, &replacement));
			DestroySwapchainKHR(device, swapchain, nullptr);
			swapchain = replacement;
		}
	}
	render_vulkan::detach();
	render_vulkan::detach();
	for (auto semaphore : ready)
		DestroySemaphore(device, semaphore, nullptr);
	DestroyFence(device, acquire_fence, nullptr);
	DestroyCommandPool(device, pool, nullptr);
	DestroySwapchainKHR(device, swapchain, nullptr);
	DestroyDevice(device, nullptr);
	DestroySurfaceKHR(instance, surface, nullptr);
	DestroyInstance(instance, nullptr);
	render_vulkan::stop_capture();
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	FreeLibrary(module);
	std::cout << (late ? "Late Vulkan " : "Startup Vulkan ") << (acquire2 ? "Acquire2 " : "Acquire1 ") << "PASS: 24 overlays, 0/1/2 waits, textures, resize, game presentation after detach/unhook\n";
}
int main(int argc, char** argv)
{
	try
	{
		WNDCLASSW cls{};
		cls.lpfnWndProc = DefWindowProcW;
		cls.hInstance = GetModuleHandleW(nullptr);
		cls.lpszClassName = L"RendererSmoke";
		RegisterClassW(&cls);
		HWND window = CreateWindowW(cls.lpszClassName, L"Renderer smoke", WS_OVERLAPPEDWINDOW, 0, 0, 160, 180, nullptr, nullptr, cls.hInstance, nullptr);
		require(window != nullptr, "Cannot create hidden test window");
		renderer ui;
		ui.m_window = window;
		g_renderer = &ui;
		const std::string mode = argc > 1 ? argv[1] : "dx11";
		if (mode == "vulkan")
			test_vulkan(window, ui);
		else if (mode == "vulkan-late")
			test_vulkan(window, ui, true);
		else if (mode == "vulkan-late2")
			test_vulkan(window, ui, true, true);
		else
			test_dx11(window, ui);
		g_renderer = nullptr;
		DestroyWindow(window);
		UnregisterClassW(cls.lpszClassName, cls.hInstance);
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
