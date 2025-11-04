#pragma once
#include "common.hpp"
#include "mono/mono.hpp"
#include "hooking/detour_hook.hpp"
#include "hooking/vmt_hook.hpp"

namespace big
{
	struct hooks
	{
		static void* convert_thread_to_fiber(void* param);

		static constexpr auto swapchain_num_funcs = 19;
		static constexpr auto swapchain_present_index = 8;
		static constexpr auto swapchain_draw_indexed_index = 12;
		static constexpr auto swapchain_resizebuffers_index = 13;
		static HRESULT APIENTRY swapchain_present(IDXGISwapChain* this_, UINT sync_interval, UINT flags);
		static HRESULT APIENTRY swapchain_resizebuffers(IDXGISwapChain* this_, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags);
		static void APIENTRY swapchain_draw_indexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);

		static LRESULT wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static BOOL set_cursor_pos(int x, int y);
		static bool is_teleportable(void* _this);
		static void create_tomb_stone(void* _this);
		static bool is_under_roof(Vector3 startPos);
		static void on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod);
		static float get_weight(void* _thisint, int stackOverride);
		static void update_water(MonoObject* _this, float dt);
		static bool is_debug_flying(MonoObject* _this);
	};

	struct minhook_keepalive
	{
		minhook_keepalive();
		~minhook_keepalive();
	};

	class hooking
	{
		friend hooks;
	public:
		explicit hooking();
		~hooking();

		void enable();
		void disable();
	private:
		bool m_enabled{};
		minhook_keepalive m_minhook_keepalive;

		WNDPROC m_og_wndproc;
	};

	inline hooking* g_hooking{};
}
