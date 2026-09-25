#include "render_vulkan.hpp"
#include "renderer.hpp"
#include <MinHook.h>
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <backends/imgui_impl_vulkan.h>

namespace big
{
	namespace
	{
		// Function pointers keep DX11 installations independent of vulkan-1.dll.
		// clang-format off
#define DEVICE_FUNCTIONS(X) \
	X(GetDeviceQueue) \
	X(GetSwapchainImagesKHR) \
	X(DeviceWaitIdle) \
	X(QueueWaitIdle) \
	X(CreateRenderPass) \
	X(DestroyRenderPass) \
	X(CreateImageView) \
	X(DestroyImageView) \
	X(CreateFramebuffer) \
	X(DestroyFramebuffer) \
	X(CreateCommandPool) \
	X(DestroyCommandPool) \
	X(AllocateCommandBuffers) \
	X(ResetCommandBuffer) \
	X(BeginCommandBuffer) \
	X(EndCommandBuffer) \
	X(CreateFence) \
	X(DestroyFence) \
	X(WaitForFences) \
	X(ResetFences) \
	X(CreateSemaphore) \
	X(DestroySemaphore) \
	X(QueueSubmit) \
	X(CmdBeginRenderPass) \
	X(CmdEndRenderPass) \
	X(CreateImage) \
	X(DestroyImage) \
	X(GetImageMemoryRequirements) \
	X(AllocateMemory) \
	X(FreeMemory) \
	X(BindImageMemory) \
	X(CreateBuffer) \
	X(DestroyBuffer) \
	X(GetBufferMemoryRequirements) \
	X(BindBufferMemory) \
	X(MapMemory) \
	X(UnmapMemory) \
	X(CreateSampler) \
	X(DestroySampler) \
	X(CmdPipelineBarrier) \
	X(CmdCopyBufferToImage)
		// clang-format on

		struct device_functions
		{
#define DECLARE(name) PFN_vk##name name{};
			DEVICE_FUNCTIONS(DECLARE)
#undef DECLARE
		} vk;
		struct hook_record
		{
			void* target;
		};
		struct queue_info
		{
			uint32_t family;
			VkQueueFlags flags;
		};
		struct frame
		{
			VkImageView view{};
			VkFramebuffer framebuffer{};
			VkCommandBuffer command{};
			VkFence fence{};
			VkSemaphore finished{};
		};
		struct texture
		{
			VkImage image{};
			VkDeviceMemory memory{};
			VkImageView view{};
			VkSampler sampler{};
			VkDescriptorSet descriptor{};
		};
		struct swapchain_info
		{
			HWND window{};
			VkFormat format{};
			VkExtent2D extent{};
			VkImageUsageFlags usage{};
			uint32_t layers{};
			bool protected_images{};
			bool inferred{};
		};
		std::recursive_mutex mutex;
		render_vulkan backend;
		std::vector<hook_record> hooks;
		PFN_vkGetInstanceProcAddr get_instance_proc{};
		PFN_vkGetDeviceProcAddr get_device_proc{};
		PFN_vkCreateInstance original_create_instance{};
		PFN_vkCreateWin32SurfaceKHR original_create_surface{};
		PFN_vkDestroySurfaceKHR original_destroy_surface{};
		PFN_vkEnumeratePhysicalDevices original_enumerate{};
		PFN_vkCreateDevice original_create_device{};
		PFN_vkCreateSwapchainKHR original_create_swapchain{};
		PFN_vkDestroySwapchainKHR original_destroy_swapchain{};
		PFN_vkQueuePresentKHR original_present{};
		PFN_vkDestroyDevice original_destroy_device{};
		PFN_vkAcquireNextImageKHR original_acquire{};
		PFN_vkAcquireNextImage2KHR original_acquire2{};
		std::unordered_map<VkPhysicalDevice, VkInstance> physical_instances;
		std::unordered_map<VkSurfaceKHR, HWND> surfaces;
		std::unordered_map<VkQueue, queue_info> queues;
		std::unordered_map<VkSwapchainKHR, swapchain_info> swapchains;
		VkInstance instance{};
		VkInstance probe_instance{};
		VkSurfaceKHR probe_surface{};
		std::vector<VkQueueFamilyProperties> probe_families;
		uint32_t probe_queue_family = UINT32_MAX;
		bool late_device = false;
		thread_local bool probing = false;
		VkPhysicalDevice physical{};
		VkDevice device{};
		VkQueue graphics_queue{};
		uint32_t queue_family{};
		VkSwapchainKHR active_swapchain{};
		VkRenderPass render_pass{};
		VkCommandPool command_pool{};
		std::vector<frame> frames;
		std::vector<texture> textures;
		bool capturing = false, attached = false, initialized = false, failed = false;
		bool minhook_owned = false, stopping = false;
		HMODULE loader_reference{};
		std::atomic<unsigned> active_callbacks{0};
		struct callback_scope
		{
			callback_scope()
			{
				active_callbacks.fetch_add(1);
			}
			~callback_scope()
			{
				active_callbacks.fetch_sub(1);
			}
		};

		void check(VkResult result)
		{
			if (result != VK_SUCCESS)
				throw std::runtime_error("Vulkan operation failed: " + std::to_string(result));
		}
		template<typename T>
		T instance_function(const char* name)
		{
			auto function = reinterpret_cast<T>(get_instance_proc(instance, name));
			if (!function)
				throw std::runtime_error(std::string("Missing Vulkan function: ") + name);
			return function;
		}
		template<typename T>
		bool install(void* target, T callback, T& original)
		{
			if (!target)
				return false;
			for (const auto& hook : hooks)
				if (hook.target == target)
					return true;
			void* trampoline = nullptr;
			if (MH_CreateHook(target, reinterpret_cast<void*>(callback), &trampoline) != MH_OK)
				return false;
			original = reinterpret_cast<T>(trampoline);
			if (MH_EnableHook(target) != MH_OK)
			{
				MH_RemoveHook(target);
				return false;
			}
			hooks.push_back({target});
			return true;
		}
		void load_device_functions()
		{
#define LOAD(name)                                                                 \
	vk.name = reinterpret_cast<PFN_vk##name>(get_device_proc(device, "vk" #name)); \
	if (!vk.name)                                                                  \
		throw std::runtime_error("Missing vk" #name);
			DEVICE_FUNCTIONS(LOAD)
#undef LOAD
		}

		void adopt_device(VkDevice current)
		{
			if (device || !probe_instance || stopping)
				return;

			device = current;
			late_device = true;
			load_device_functions();

			// Unlike querying every queue advertised by the GPU, this only queries
			// queue zero of a unique graphics family. A graphics device must request
			// that family to render. Ambiguous family layouts are rejected at bootstrap.
			VkQueue queue = VK_NULL_HANDLE;
			vk.GetDeviceQueue(device, probe_queue_family, 0, &queue);
			if (!queue)
				throw std::runtime_error("Cannot resolve the game's Vulkan graphics queue");

			queues[queue] = {probe_queue_family, probe_families[probe_queue_family].queueFlags};
			LOG(INFO) << "Captured existing Vulkan device from swapchain hook.";
		}

		void describe_existing_swapchain(VkSwapchainKHR swapchain)
		{
			if (swapchains.contains(swapchain) || !g_renderer || !late_device)
				return;

			VkSurfaceCapabilitiesKHR capabilities{};
			check(instance_function<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")(physical, probe_surface, &capabilities));
			VkExtent2D extent = capabilities.currentExtent;
			if (extent.width == UINT32_MAX)
			{
				RECT rect{};
				if (!GetClientRect(g_renderer->m_window, &rect))
					throw std::runtime_error("Cannot read Vulkan window size");
				extent = {static_cast<uint32_t>(rect.right - rect.left), static_cast<uint32_t>(rect.bottom - rect.top)};
			}

			// Vulkan cannot query the creation description of an existing swapchain.
			// Use RGBA8 UNORM provisionally with the game window size. Capture
			// CreateSwapchainKHR thereafter to replace this provisional description.
			uint32_t count = 0;
			auto get_formats = instance_function<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR");
			check(get_formats(physical, probe_surface, &count, nullptr));
			std::vector<VkSurfaceFormatKHR> formats(count);
			check(get_formats(physical, probe_surface, &count, formats.data()));
			bool supports_rgba = false;
			for (const auto& format : formats)
				supports_rgba |= format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_UNDEFINED;
			if (!supports_rgba)
				throw std::runtime_error("Existing swapchain needs recreation: RGBA8 UNORM fallback is not supported");

			swapchains[swapchain] = {g_renderer->m_window, VK_FORMAT_R8G8B8A8_UNORM, extent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1, false, true};
			LOG(INFO) << "Late Vulkan attachment: using RGBA8 UNORM and current window size until swapchain recreation.";
		}

		uint32_t memory_type(uint32_t bits, VkMemoryPropertyFlags required)
		{
			VkPhysicalDeviceMemoryProperties properties{};
			instance_function<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties")(physical, &properties);
			for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
				if ((bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & required) == required)
					return i;
			throw std::runtime_error("No compatible Vulkan memory type");
		}
		void free_texture(texture& t)
		{
			if (t.descriptor)
				ImGui_ImplVulkan_RemoveTexture(t.descriptor);
			if (t.sampler)
				vk.DestroySampler(device, t.sampler, nullptr);
			if (t.view)
				vk.DestroyImageView(device, t.view, nullptr);
			if (t.image)
				vk.DestroyImage(device, t.image, nullptr);
			if (t.memory)
				vk.FreeMemory(device, t.memory, nullptr);
			t = {};
		}
		void cleanup()
		{
			std::lock_guard ui_lock(render_mutex);
			if (!device || !vk.DeviceWaitIdle)
				return;
			vk.DeviceWaitIdle(device);
			for (auto& t : textures)
				free_texture(t);
			textures.clear();
			if (g_renderer && g_renderer->api() == renderer_api::vulkan && ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData)
				ImGui_ImplVulkan_Shutdown();
			initialized = false;
			for (auto& f : frames)
			{
				if (f.framebuffer)
					vk.DestroyFramebuffer(device, f.framebuffer, nullptr);
				if (f.view)
					vk.DestroyImageView(device, f.view, nullptr);
				if (f.fence)
					vk.DestroyFence(device, f.fence, nullptr);
				if (f.finished)
					vk.DestroySemaphore(device, f.finished, nullptr);
			}
			frames.clear();
			if (command_pool)
				vk.DestroyCommandPool(device, command_pool, nullptr);
			if (render_pass)
				vk.DestroyRenderPass(device, render_pass, nullptr);
			command_pool = {};
			render_pass = {};
			active_swapchain = {};
			if (g_renderer)
				g_renderer->release_vulkan();
		}
		PFN_vkVoidFunction imgui_loader(const char* name, void*)
		{
			if (auto fn = get_device_proc(device, name))
				return fn;
			return get_instance_proc(instance, name);
		}
		void initialize(VkQueue queue, VkSwapchainKHR swapchain)
		{
			const auto& description = swapchains.at(swapchain);
			graphics_queue = queue;
			queue_family = queues.at(queue).family;
			uint32_t count = 0;
			check(vk.GetSwapchainImagesKHR(device, swapchain, &count, nullptr));
			if (count < 2)
				throw std::runtime_error("Vulkan overlay needs at least two swapchain images");
			std::vector<VkImage> images(count);
			check(vk.GetSwapchainImagesKHR(device, swapchain, &count, images.data()));
			images.resize(count);
			frames.resize(count);
			VkAttachmentDescription attachment{};
			attachment.format = description.format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &reference;
			VkSubpassDependency dependencies[2]{};
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
			rp.attachmentCount = 1;
			rp.pAttachments = &attachment;
			rp.subpassCount = 1;
			rp.pSubpasses = &subpass;
			rp.dependencyCount = 2;
			rp.pDependencies = dependencies;
			check(vk.CreateRenderPass(device, &rp, nullptr, &render_pass));
			VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool.queueFamilyIndex = queue_family;
			check(vk.CreateCommandPool(device, &pool, nullptr, &command_pool));
			for (uint32_t i = 0; i < count; ++i)
			{
				auto& f = frames[i];
				VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
				view.image = images[i];
				view.viewType = VK_IMAGE_VIEW_TYPE_2D;
				view.format = description.format;
				view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
				check(vk.CreateImageView(device, &view, nullptr, &f.view));
				VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
				fb.renderPass = render_pass;
				fb.attachmentCount = 1;
				fb.pAttachments = &f.view;
				fb.width = description.extent.width;
				fb.height = description.extent.height;
				fb.layers = 1;
				check(vk.CreateFramebuffer(device, &fb, nullptr, &f.framebuffer));
				VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
				allocate.commandPool = command_pool;
				allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocate.commandBufferCount = 1;
				check(vk.AllocateCommandBuffers(device, &allocate, &f.command));
				VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
				fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				check(vk.CreateFence(device, &fence, nullptr, &f.fence));
				VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
				check(vk.CreateSemaphore(device, &semaphore, nullptr, &f.finished));
			}
			if (!g_renderer->begin_vulkan(&backend))
				throw std::runtime_error("Another renderer is active");
			// Load only Vulkan 1.0 functions: no optional extension is assumed enabled.
			if (!ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_0, imgui_loader))
				throw std::runtime_error("Cannot load ImGui Vulkan functions");
			ImGui_ImplVulkan_InitInfo info{};
			info.ApiVersion = VK_API_VERSION_1_0;
			info.Instance = instance;
			info.PhysicalDevice = physical;
			info.Device = device;
			info.QueueFamily = queue_family;
			info.Queue = queue;
			info.RenderPass = render_pass;
			info.MinImageCount = 2;
			info.ImageCount = count;
			info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			info.DescriptorPoolSize = 256;
			info.CheckVkResultFn = check;
			if (!ImGui_ImplVulkan_Init(&info))
				throw std::runtime_error("Cannot initialize ImGui Vulkan backend");
			initialized = true;
			active_swapchain = swapchain;
			g_renderer->finish_init();
			LOG(INFO) << "Active renderer: Vulkan; swapchain images=" << count;
		}
		VkResult VKAPI_CALL present(VkQueue queue, const VkPresentInfoKHR* info)
		{
			callback_scope callback;
			std::lock_guard lock(mutex);
			std::lock_guard ui_lock(render_mutex);
			if (!attached || !g_running || !g_renderer || failed || !info || info->swapchainCount != 1 || g_renderer->api() == renderer_api::dx11 || !queues.contains(queue) || !(queues.at(queue).flags & VK_QUEUE_GRAPHICS_BIT) || !swapchains.contains(info->pSwapchains[0]))
				return original_present(queue, info);
			const auto swapchain = info->pSwapchains[0];
			const auto& description = swapchains.at(swapchain);
			if (description.window != g_renderer->m_window || !description.extent.width || !description.extent.height || description.layers != 1 || description.protected_images || !(description.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
				return original_present(queue, info);
			if (active_swapchain && (active_swapchain != swapchain || graphics_queue != queue))
				return original_present(queue, info);
			bool submitted = false;
			VkPresentInfoKHR overlay_present = *info;
			VkSemaphore finished{};
			try
			{
				if (!initialized)
					initialize(queue, swapchain);
				const uint32_t index = info->pImageIndices[0];
				if (index >= frames.size())
					return original_present(queue, info);
				auto& f = frames[index];
				check(vk.WaitForFences(device, 1, &f.fence, VK_TRUE, UINT64_MAX));
				check(vk.ResetCommandBuffer(f.command, 0));
				ImGui_ImplVulkan_NewFrame();
				g_renderer->draw_frame();
				// Font atlas updates may submit their own upload before recording the draw.
				VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
				begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				check(vk.BeginCommandBuffer(f.command, &begin));
				VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
				pass.renderPass = render_pass;
				pass.framebuffer = f.framebuffer;
				pass.renderArea.extent = description.extent;
				vk.CmdBeginRenderPass(f.command, &pass, VK_SUBPASS_CONTENTS_INLINE);
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), f.command);
				vk.CmdEndRenderPass(f.command);
				check(vk.EndCommandBuffer(f.command));
				std::vector<VkPipelineStageFlags> stages(info->waitSemaphoreCount, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
				VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
				submit.waitSemaphoreCount = info->waitSemaphoreCount;
				submit.pWaitSemaphores = info->pWaitSemaphores;
				submit.pWaitDstStageMask = stages.data();
				submit.commandBufferCount = 1;
				submit.pCommandBuffers = &f.command;
				submit.signalSemaphoreCount = 1;
				submit.pSignalSemaphores = &f.finished;
				check(vk.ResetFences(device, 1, &f.fence));
				check(vk.QueueSubmit(queue, 1, &submit, f.fence));
				submitted = true;
				finished = f.finished;
				// Original waits were consumed by the overlay submit. Present waits only on its completion.
				overlay_present.waitSemaphoreCount = 1;
				overlay_present.pWaitSemaphores = &finished;
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << "Vulkan overlay disabled: " << e.what();
				failed = true;
			}
			const auto result = original_present(queue, submitted ? &overlay_present : info);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_DEVICE_LOST)
				failed = true;
			return result;
		}
		VkResult VKAPI_CALL create_swapchain(VkDevice current, const VkSwapchainCreateInfoKHR* info,
		    const VkAllocationCallbacks* allocator, VkSwapchainKHR* result)
		{
			callback_scope callback;
			std::lock_guard lock(mutex);
			auto status = original_create_swapchain(current, info, allocator, result);
			if (status == VK_SUCCESS && !stopping && (current == device || (!device && probe_instance)))
			{
				try
				{
					adopt_device(current);
				}
				catch (const std::exception& e)
				{
					LOG(WARNING) << "Cannot attach Vulkan swapchain: " << e.what();
					failed = true;
					return status;
				}
				if (info->oldSwapchain && info->oldSwapchain == active_swapchain)
					cleanup();
				swapchains[*result] = {surfaces.contains(info->surface) ? surfaces.at(info->surface) : (late_device && g_renderer ? g_renderer->m_window : nullptr), info->imageFormat, info->imageExtent, info->imageUsage, info->imageArrayLayers, bool(info->flags & VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR)};
				failed = false;
			}
			return status;
		}
		void VKAPI_CALL destroy_swapchain(VkDevice current, VkSwapchainKHR swapchain, const VkAllocationCallbacks* allocator)
		{
			callback_scope callback;
			std::lock_guard lock(mutex);
			if (current == device)
			{
				if (active_swapchain == swapchain)
					cleanup();
				swapchains.erase(swapchain);
			}
			original_destroy_swapchain(current, swapchain, allocator);
		}
		void VKAPI_CALL destroy_device(VkDevice current, const VkAllocationCallbacks* allocator)
		{
			callback_scope callback;
			std::lock_guard lock(mutex);
			if (current == device)
			{
				cleanup();
				device = {};
				queues.clear();
				swapchains.clear();
			}
			original_destroy_device(current, allocator);
		}
		VkResult VKAPI_CALL acquire_next_image(VkDevice current, VkSwapchainKHR swapchain, uint64_t timeout,
		    VkSemaphore semaphore, VkFence fence, uint32_t* index)
		{
			callback_scope callback;
			// Acquisition can block waiting for presentation. Never hold our lock here.
			const auto result = original_acquire(current, swapchain, timeout, semaphore, fence, index);
			if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
			{
				std::lock_guard lock(mutex);
				if (!stopping && (current == device || !device))
				{
					try
					{
						adopt_device(current);
						describe_existing_swapchain(swapchain);
					}
					catch (const std::exception& e)
					{
						LOG(WARNING) << "Vulkan acquisition capture failed: " << e.what();
						failed = true;
					}
				}
			}
			return result;
		}

		VkResult VKAPI_CALL acquire_next_image2(VkDevice current, const VkAcquireNextImageInfoKHR* info, uint32_t* index)
		{
			callback_scope callback;
			const auto result = original_acquire2(current, info, index);
			if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
			{
				std::lock_guard lock(mutex);
				if (!stopping && (current == device || !device))
				{
					try
					{
						adopt_device(current);
						describe_existing_swapchain(info->swapchain);
					}
					catch (const std::exception& e)
					{
						LOG(WARNING) << "Vulkan acquisition capture failed: " << e.what();
						failed = true;
					}
				}
			}
			return result;
		}

		void install_device_hooks(VkDevice current)
		{
			bool ok = install(reinterpret_cast<void*>(get_device_proc(current, "vkCreateSwapchainKHR")), create_swapchain, original_create_swapchain);
			ok &= install(reinterpret_cast<void*>(get_device_proc(current, "vkDestroySwapchainKHR")), destroy_swapchain, original_destroy_swapchain);
			ok &= install(reinterpret_cast<void*>(get_device_proc(current, "vkQueuePresentKHR")), present, original_present);
			ok &= install(reinterpret_cast<void*>(get_device_proc(current, "vkDestroyDevice")), destroy_device, original_destroy_device);
			ok &= install(reinterpret_cast<void*>(get_device_proc(current, "vkAcquireNextImageKHR")), acquire_next_image, original_acquire);
			if (auto acquire2 = get_device_proc(current, "vkAcquireNextImage2KHR"))
				ok &= install(reinterpret_cast<void*>(acquire2), acquire_next_image2, original_acquire2);
			if (!ok)
				throw std::runtime_error("Cannot install Vulkan device hooks");
		}

		VkResult VKAPI_CALL create_device(VkPhysicalDevice gpu, const VkDeviceCreateInfo* info,
		    const VkAllocationCallbacks* allocator, VkDevice* result)
		{
			callback_scope callback;
			auto status = original_create_device(gpu, info, allocator, result);
			if (status != VK_SUCCESS)
				return status;
			std::lock_guard lock(mutex);
			bool supports_swapchain = false;
			for (uint32_t i = 0; i < info->enabledExtensionCount; ++i)
				supports_swapchain |= std::strcmp(info->ppEnabledExtensionNames[i], "VK_KHR_swapchain") == 0;
			if (probing || stopping || device || !supports_swapchain || !physical_instances.contains(gpu))
				return status;
			instance = physical_instances.at(gpu);
			physical = gpu;
			device = *result;
			try
			{
				load_device_functions();
				auto properties = instance_function<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
				uint32_t count = 0;
				properties(gpu, &count, nullptr);
				std::vector<VkQueueFamilyProperties> families(count);
				properties(gpu, &count, families.data());
				for (uint32_t i = 0; i < info->queueCreateInfoCount; ++i)
				{
					const auto& q = info->pQueueCreateInfos[i];
					if (q.flags || q.queueFamilyIndex >= count)
						continue;
					for (uint32_t j = 0; j < q.queueCount; ++j)
					{
						VkQueue queue{};
						vk.GetDeviceQueue(device, q.queueFamilyIndex, j, &queue);
						queues[queue] = {q.queueFamilyIndex, families[q.queueFamilyIndex].queueFlags};
					}
				}
				late_device = false;
				install_device_hooks(device);
			}
			catch (const std::exception&)
			{
				// Capture can run before the logger exists. Report its status on attach.
				failed = true;
			}
			return status;
		}
		VkResult VKAPI_CALL enumerate(VkInstance current, uint32_t* count, VkPhysicalDevice* devices)
		{
			callback_scope callback;
			auto status = original_enumerate(current, count, devices);
			if ((status == VK_SUCCESS || status == VK_INCOMPLETE) && devices)
			{
				std::lock_guard lock(mutex);
				for (uint32_t i = 0; i < *count; ++i)
					physical_instances[devices[i]] = current;
			}
			return status;
		}
		VkResult VKAPI_CALL create_surface(VkInstance current, const VkWin32SurfaceCreateInfoKHR* info,
		    const VkAllocationCallbacks* allocator, VkSurfaceKHR* result)
		{
			callback_scope callback;
			auto status = original_create_surface(current, info, allocator, result);
			if (status == VK_SUCCESS)
			{
				std::lock_guard lock(mutex);
				surfaces[*result] = info->hwnd;
			}
			return status;
		}
		void VKAPI_CALL destroy_surface(VkInstance current, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator)
		{
			callback_scope callback;
			std::lock_guard lock(mutex);
			surfaces.erase(surface);
			original_destroy_surface(current, surface, allocator);
		}
		VkResult VKAPI_CALL create_instance(const VkInstanceCreateInfo* info, const VkAllocationCallbacks* allocator, VkInstance* result)
		{
			callback_scope callback;
			auto status = original_create_instance(info, allocator, result);
			if (status == VK_SUCCESS)
			{
				std::lock_guard lock(mutex);
				if (!stopping)
				{
					auto create = get_instance_proc(*result, "vkCreateWin32SurfaceKHR");
					auto destroy = get_instance_proc(*result, "vkDestroySurfaceKHR");
					if (create)
						install(reinterpret_cast<void*>(create), create_surface, original_create_surface);
					if (destroy)
						install(reinterpret_cast<void*>(destroy), destroy_surface, original_destroy_surface);
				}
			}
			return status;
		}
		void bootstrap(uint32_t vendor_id, uint32_t device_id)
		{
			if (device || probe_instance)
				return;

			struct probe_scope
			{
				probe_scope()
				{
					probing = true;
				}
				~probe_scope()
				{
					probing = false;
				}
			} scope;

			VkDevice dummy = VK_NULL_HANDLE;
			try
			{
				uint32_t version = VK_API_VERSION_1_0;
				if (auto enumerate_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(get_instance_proc(nullptr, "vkEnumerateInstanceVersion")))
				{
					check(enumerate_version(&version));
					version = (std::min)(version, VK_API_VERSION_1_1);
				}
				VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
				application.apiVersion = version;
				const char* extensions[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
				VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
				info.pApplicationInfo = &application;
				info.enabledExtensionCount = 2;
				info.ppEnabledExtensionNames = extensions;
				check(original_create_instance(&info, nullptr, &probe_instance));
				instance = probe_instance;

				uint32_t count = 0;
				check(original_enumerate(probe_instance, &count, nullptr));
				std::vector<VkPhysicalDevice> devices(count);
				check(original_enumerate(probe_instance, &count, devices.data()));
				auto get_properties = instance_function<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties");
				for (auto gpu : devices)
				{
					VkPhysicalDeviceProperties properties{};
					get_properties(gpu, &properties);
					if (vendor_id && (properties.vendorID != vendor_id || properties.deviceID != device_id))
						continue;
					if (!physical || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						physical = gpu;
					if (vendor_id || properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
						break;
				}
				if (!physical)
					throw std::runtime_error("Cannot find the game's Vulkan GPU");

				auto get_families = instance_function<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
				get_families(physical, &count, nullptr);
				probe_families.resize(count);
				get_families(physical, &count, probe_families.data());
				probe_queue_family = UINT32_MAX;
				for (uint32_t i = 0; i < count; ++i)
				{
					if (!(probe_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
						continue;
					if (probe_queue_family != UINT32_MAX)
						throw std::runtime_error("Late Vulkan attachment requires an unambiguous graphics queue family");
					probe_queue_family = i;
				}
				if (probe_queue_family == UINT32_MAX)
					throw std::runtime_error("No Vulkan graphics queue family");

				VkWin32SurfaceCreateInfoKHR surface_info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
				surface_info.hinstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_renderer->m_window, GWLP_HINSTANCE));
				surface_info.hwnd = g_renderer->m_window;
				check(instance_function<PFN_vkCreateWin32SurfaceKHR>("vkCreateWin32SurfaceKHR")(probe_instance, &surface_info, nullptr, &probe_surface));
				VkBool32 supported = VK_FALSE;
				check(instance_function<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>("vkGetPhysicalDeviceSurfaceSupportKHR")(physical, probe_queue_family, probe_surface, &supported));
				if (!supported)
					throw std::runtime_error("Graphics queue cannot present to the game window");

				constexpr float priority = 1.0f;
				VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
				queue_info.queueFamilyIndex = probe_queue_family;
				queue_info.queueCount = 1;
				queue_info.pQueuePriorities = &priority;
				const char* extension = "VK_KHR_swapchain";
				VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
				device_info.queueCreateInfoCount = 1;
				device_info.pQueueCreateInfos = &queue_info;
				device_info.enabledExtensionCount = 1;
				device_info.ppEnabledExtensionNames = &extension;
				check(original_create_device(physical, &device_info, nullptr, &dummy));
				install_device_hooks(dummy);
				original_destroy_device(dummy, nullptr);
				dummy = VK_NULL_HANDLE;
				VkPhysicalDeviceProperties properties{};
				get_properties(physical, &properties);
				LOG(INFO) << "Vulkan dummy device hooks initialized on " << properties.deviceName << "; waiting for game acquisition.";
			}
			catch (...)
			{
				if (dummy)
					reinterpret_cast<PFN_vkDestroyDevice>(get_device_proc(dummy, "vkDestroyDevice"))(dummy, nullptr);
				// Keep the probe instance alive if some hooks already reference its driver.
				// stop_capture() owns its cleanup after all hooks have been removed.
				throw;
			}
		}

	}
	void render_vulkan::start_capture()
	{
		std::lock_guard lock(mutex);
		if (capturing || stopping)
			return;
		// Prime the system loader before Unity resolves device-creation functions.
		// Loading is optional; DX11 continues when the Vulkan runtime is absent.
		loader_reference = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		const auto module = loader_reference;
		if (!module)
			return;
		const auto mh = MH_Initialize();
		if (mh != MH_OK && mh != MH_ERROR_ALREADY_INITIALIZED)
		{
			FreeLibrary(loader_reference);
			loader_reference = nullptr;
			return;
		}
		minhook_owned = mh == MH_OK;
		get_instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(module, "vkGetInstanceProcAddr"));
		get_device_proc = reinterpret_cast<PFN_vkGetDeviceProcAddr>(GetProcAddress(module, "vkGetDeviceProcAddr"));
		if (!get_instance_proc || !get_device_proc)
		{
			FreeLibrary(loader_reference);
			loader_reference = nullptr;
			return;
		}
		bool ok = install(reinterpret_cast<void*>(GetProcAddress(module, "vkCreateInstance")), create_instance, original_create_instance);
		ok &= install(reinterpret_cast<void*>(GetProcAddress(module, "vkEnumeratePhysicalDevices")), enumerate, original_enumerate);
		ok &= install(reinterpret_cast<void*>(GetProcAddress(module, "vkCreateDevice")), create_device, original_create_device);
		capturing = true;
		failed = !ok;
	}
	void render_vulkan::attach(uint32_t vendor_id, uint32_t device_id)
	{
		start_capture();
		std::lock_guard lock(mutex);
		attached = true;
		if (capturing && !failed && !device && g_renderer)
		{
			try
			{
				bootstrap(vendor_id, device_id);
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << "Vulkan bootstrap failed: " << e.what();
				failed = true;
			}
		}
	}
	void render_vulkan::detach()
	{
		std::lock_guard lock(mutex);
		attached = false;
		cleanup();
	}
	void render_vulkan::stop_capture()
	{
		detach();
		std::vector<hook_record> installed;
		{
			std::lock_guard lock(mutex);
			stopping = true;
			installed = hooks;
		}
		for (const auto& hook : installed)
			MH_DisableHook(hook.target);
		// Do not release a trampoline while a callback is still returning through it.
		while (active_callbacks.load() != 0)
			std::this_thread::yield();
		std::lock_guard lock(mutex);
		for (const auto& hook : installed)
			MH_RemoveHook(hook.target);
		hooks.clear();
		queues.clear();
		swapchains.clear();
		surfaces.clear();
		physical_instances.clear();
		if (probe_instance)
		{
			if (probe_surface)
				reinterpret_cast<PFN_vkDestroySurfaceKHR>(get_instance_proc(probe_instance, "vkDestroySurfaceKHR"))(probe_instance, probe_surface, nullptr);
			reinterpret_cast<PFN_vkDestroyInstance>(get_instance_proc(probe_instance, "vkDestroyInstance"))(probe_instance, nullptr);
		}
		probe_instance = VK_NULL_HANDLE;
		probe_surface = VK_NULL_HANDLE;
		probe_families.clear();
		probe_queue_family = UINT32_MAX;
		late_device = false;
		failed = false;
		device = {};
		physical = {};
		instance = {};
		vk = {};
		capturing = false;
		if (minhook_owned)
			MH_Uninitialize();
		minhook_owned = false;
		if (loader_reference)
			FreeLibrary(loader_reference);
		loader_reference = nullptr;
		stopping = false;
	}
	void render_vulkan::shutdown()
	{
		detach();
	}
	void render_vulkan::release_texture(ImTextureID id)
	{
		std::lock_guard lock(mutex);
		auto it = std::find_if(textures.begin(), textures.end(), [id](const auto& t) {
			return reinterpret_cast<ImTextureID>(t.descriptor) == id;
		});
		if (it == textures.end())
			return;
		if (vk.DeviceWaitIdle(device) != VK_SUCCESS)
			return;
		free_texture(*it);
		textures.erase(it);
	}
	ImTextureID render_vulkan::upload_rgba(const unsigned char* pixels, int width, int height)
	{
		std::lock_guard lock(mutex);
		if (!initialized || !pixels || width <= 0 || height <= 0)
			return 0;
		texture t{};
		VkBuffer staging{};
		VkDeviceMemory staging_memory{};
		VkCommandPool upload_pool{};
		try
		{
			const VkDeviceSize size = VkDeviceSize(width) * VkDeviceSize(height) * 4;
			VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
			buffer.size = size;
			buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			check(vk.CreateBuffer(device, &buffer, nullptr, &staging));
			VkMemoryRequirements requirements{};
			vk.GetBufferMemoryRequirements(device, staging, &requirements);
			VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
			allocate.allocationSize = requirements.size;
			allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			check(vk.AllocateMemory(device, &allocate, nullptr, &staging_memory));
			check(vk.BindBufferMemory(device, staging, staging_memory, 0));
			void* mapped = nullptr;
			check(vk.MapMemory(device, staging_memory, 0, size, 0, &mapped));
			std::memcpy(mapped, pixels, static_cast<size_t>(size));
			vk.UnmapMemory(device, staging_memory);
			VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
			image.imageType = VK_IMAGE_TYPE_2D;
			image.format = VK_FORMAT_R8G8B8A8_UNORM;
			image.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
			image.mipLevels = image.arrayLayers = 1;
			image.samples = VK_SAMPLE_COUNT_1_BIT;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			check(vk.CreateImage(device, &image, nullptr, &t.image));
			vk.GetImageMemoryRequirements(device, t.image, &requirements);
			allocate.allocationSize = requirements.size;
			allocate.memoryTypeIndex = memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			check(vk.AllocateMemory(device, &allocate, nullptr, &t.memory));
			check(vk.BindImageMemory(device, t.image, t.memory, 0));
			VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			pool.queueFamilyIndex = queue_family;
			pool.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			check(vk.CreateCommandPool(device, &pool, nullptr, &upload_pool));
			VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			command_info.commandPool = upload_pool;
			command_info.commandBufferCount = 1;
			command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			VkCommandBuffer command{};
			check(vk.AllocateCommandBuffers(device, &command_info, &command));
			VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			check(vk.BeginCommandBuffer(command, &begin));
			VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = t.image;
			barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vk.CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			VkBufferImageCopy copy{};
			copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			copy.imageExtent = image.extent;
			vk.CmdCopyBufferToImage(command, staging, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vk.CmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			check(vk.EndCommandBuffer(command));
			VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &command;
			check(vk.QueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE));
			check(vk.QueueWaitIdle(graphics_queue));
			VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			view.image = t.image;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = image.format;
			view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			check(vk.CreateImageView(device, &view, nullptr, &t.view));
			VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
			sampler.magFilter = sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			check(vk.CreateSampler(device, &sampler, nullptr, &t.sampler));
			t.descriptor = ImGui_ImplVulkan_AddTexture(t.sampler, t.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			if (!t.descriptor)
				throw std::runtime_error("Cannot allocate menu texture descriptor");
			textures.push_back(t);
		}
		catch (const std::exception& e)
		{
			LOG(WARNING) << "Vulkan texture upload failed: " << e.what();
			vk.QueueWaitIdle(graphics_queue);
			free_texture(t);
		}
		if (upload_pool)
			vk.DestroyCommandPool(device, upload_pool, nullptr);
		if (staging)
			vk.DestroyBuffer(device, staging, nullptr);
		if (staging_memory)
			vk.FreeMemory(device, staging_memory, nullptr);
		return reinterpret_cast<ImTextureID>(t.descriptor);
	}
}
