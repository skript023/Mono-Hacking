#pragma once
#include "render_backend.hpp"
#include <wrl/client.h>
namespace big
{
	class render_dx11 final : public render_backend
	{
		Microsoft::WRL::ComPtr<ID3D11Device> m_device;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_target;
		Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapchain;
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textures;
		bool m_initialized = false;
		bool m_resizing = false;

	public:
		~render_dx11() override
		{
			shutdown();
		}
		bool init(IDXGISwapChain* swapchain);
		void present();
		void pre_reset();
		void post_reset(IDXGISwapChain* swapchain);
		void shutdown() override;
		bool owns(IDXGISwapChain* swapchain) const
		{
			return m_swapchain.Get() == swapchain;
		}
		ImTextureID upload_rgba(const unsigned char* pixels, int width, int height) override;
	};
}
