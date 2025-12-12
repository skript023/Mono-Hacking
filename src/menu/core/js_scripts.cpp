#include "../view.hpp"
#include "renderer.hpp"
#include "fiber_pool.hpp"
#include "commands/bool_command.hpp"
#include "javascript/javascript_manager.hpp"

namespace big
{
	void view::js_scripts()
	{
		if (!g_settings.window.js_eval)
		{
			if (auto cmd = commands::get_command<bool_command>(joaat("disable_input")))
			{
				cmd->set_state(false);
			}

			return;
		}

		ImGui::SetNextWindowSize({650, 350}, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Javascript Console"))
		{
			ImGui::PushFont(g_renderer->m_monospace_font);

			static std::string input_code;

			// ============================
			//  SECTION: EXECUTOR
			// ============================
			ImGui::Text("Javascript Executor");
			ImGui::Separator();

			ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;

			if (ImGui::InputTextMultiline(
				"##js_exec",
				&input_code,
				ImVec2(ImGui::GetContentRegionAvail().x, 120),
				flags))
			{
				if (auto cmd = commands::get_command<bool_command>(joaat("disable_input")))
				{
					cmd->set_state(true);
				}
			}

			if (ImGui::Button("Execute", {80, 25}))
			{
				g_fiber_pool->queue_job([] {
					javascript_manager::eval_script(input_code);
				});
			}

			ImGui::PopFont();
			ImGui::End();
		}
	}
}