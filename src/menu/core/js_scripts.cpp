#include "../view.hpp"
#include "renderer.hpp"
#include "fiber_pool.hpp"
#include "commands/bool_command.hpp"
#include "javascript/javascript_manager.hpp"

namespace big
{
	static int selected_script = -1;
	static std::vector<std::filesystem::path> script_files;

	static void scan_js_scripts()
	{
		script_files.clear();

		const auto dir = file_manager::get_project_folder("./scripts");
		if (!std::filesystem::exists(dir.get_path()))
			return;

		for (auto& it : std::filesystem::directory_iterator(dir.get_path()))
		{
			if (it.is_regular_file() && it.path().extension() == ".js")
				script_files.push_back(it.path());
		}
	}

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

			ImGui::Text("JS Scripts");

			if (ImGui::Button("Reload"))
				scan_js_scripts();

			ImGui::SameLine();
			if (ImGui::Button("Open Folder"))
				std::system("explorer.exe ./Valheim\\ Mod\\scripts");

			if (ImGui::BeginListBox("##js_scripts", ImVec2(200, 200)))
			{
				for (int i = 0; i < (int)script_files.size(); ++i)
				{
					const auto& path = script_files[i];
					const std::string name = path.filename().string();

					if (ImGui::Selectable(name.c_str(), selected_script == i))
						selected_script = i;
				}
				ImGui::EndListBox();
			}

			if (selected_script != -1)
			{
				const auto& path = script_files[selected_script];

				if (ImGui::Button("Load"))
				{
					LOG(VERBOSE) << "Executing JS script: " << path.string();
					g_fiber_pool->queue_job([path] {
						javascript_manager::eval_file(path);
					});
				}
			}

			ImGui::PopFont();
			ImGui::End();
		}
	}
}