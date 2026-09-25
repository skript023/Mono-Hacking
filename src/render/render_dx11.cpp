#include "render_dx11.hpp"
#include "renderer.hpp"
#include <backends/imgui_impl_dx11.h>
namespace big
{
	bool render_dx11::init(IDXGISwapChain* swapchain)
	{
		if (m_initialized)
			return owns(swapchain);
		if (FAILED(swapchain->GetDevice(IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
			return false;
		m_device->GetImmediateContext(m_context.ReleaseAndGetAddressOf());
		m_swapchain = swapchain;
		post_reset(swapchain);
		if (!m_target || !ImGui_ImplDX11_Init(m_device.Get(), m_context.Get()))
		{
			shutdown();
			return false;
		}
		m_initialized = true;
		return true;
	}
	void render_dx11::present()
	{
		if (m_resizing)
			return;
		if (!m_target)
			post_reset(m_swapchain.Get());
		if (!m_initialized || !m_target)
			return;
		ImGui_ImplDX11_NewFrame();
		g_renderer->draw_frame();
		ID3D11RenderTargetView* previous[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
		ID3D11DepthStencilView* depth = nullptr;
		m_context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, &depth);
		auto target = m_target.Get();
		m_context->OMSetRenderTargets(1, &target, nullptr);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		m_context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, previous, depth);
		for (auto view : previous)
			if (view)
				view->Release();
		if (depth)
			depth->Release();
	}
	void render_dx11::pre_reset()
	{
		m_resizing = true;
		if (m_context)
			m_context->OMSetRenderTargets(0, nullptr, nullptr);
		m_target.Reset();
	}
	void render_dx11::post_reset(IDXGISwapChain* swapchain)
	{
		m_resizing = false;
		if (!m_device || !swapchain)
			return;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer;
		if (SUCCEEDED(swapchain->GetBuffer(0, IID_PPV_ARGS(buffer.GetAddressOf()))))
			m_device->CreateRenderTargetView(buffer.Get(), nullptr, m_target.ReleaseAndGetAddressOf());
	}
	void render_dx11::shutdown()
	{
		if (m_initialized)
			ImGui_ImplDX11_Shutdown();
		m_initialized = false;
		m_textures.clear();
		m_target.Reset();
		m_context.Reset();
		m_device.Reset();
		m_swapchain.Reset();
	}
	void render_dx11::release_texture(ImTextureID texture)
	{
		std::erase_if(m_textures, [texture](const auto& view) {
			return reinterpret_cast<ImTextureID>(view.Get()) == texture;
		});
	}
	ImTextureID render_dx11::upload_rgba(const unsigned char* pixels, int width, int height)
	{
		if (!m_device || !pixels || width <= 0 || height <= 0)
			return 0;
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		D3D11_SUBRESOURCE_DATA data{pixels, static_cast<UINT>(width * 4), 0};
		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
		if (FAILED(m_device->CreateTexture2D(&desc, &data, &texture)) || FAILED(m_device->CreateShaderResourceView(texture.Get(), nullptr, &view)))
			return 0;
		const auto id = reinterpret_cast<ImTextureID>(view.Get());
		m_textures.push_back(std::move(view));
		return id;
	}
}
