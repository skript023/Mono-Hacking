#include "../view.hpp"
#include "commands/bool_command.hpp"
#include "javascript/javascript_manager.hpp"
#include "renderer.hpp"
#include "task_mgr.hpp"

#include <shellapi.h>

namespace big
{
	namespace
	{
		std::vector<std::filesystem::path> script_files;
		int selected_script = -1;

		void scan_js_scripts()
		{
			script_files.clear();
			const auto dir = file_manager::get_project_folder("./scripts");
			if (!std::filesystem::exists(dir.get_path()))
			{
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(dir.get_path(), std::filesystem::directory_options::skip_permission_denied))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".js")
				{
					script_files.push_back(entry.path());
				}
			}

			std::ranges::sort(script_files, {}, [](const auto& path) {
				return path.filename().wstring();
			});
			if (selected_script >= static_cast<int>(script_files.size()))
			{
				selected_script = -1;
			}
		}

		std::string read_script(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
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

		static std::string input_code = "console.log('Hello from QuickJS');";
		static int snippet            = 0;
		static bool auto_scroll       = true;
		static bool scripts_scanned   = false;
		static bool show_info         = true;
		static bool show_warn         = true;
		static bool show_error        = true;
		static bool show_eval         = true;
		static ImGuiTextFilter output_filter;
		static ImGuiTextFilter script_filter;

		if (!scripts_scanned)
		{
			scan_js_scripts();
			scripts_scanned = true;
		}

		const bool visible = canvas::begin_panel("Javascript Console", &g_settings.window.js_eval, {.size = {850.f, 620.f}, .min_size = {650.f, 480.f}, .collapsible = false});
		if (visible)
		{
			const bool has_ui_font = g_renderer && g_renderer->m_font;
			if (has_ui_font)
			{
				ImGui::PushFont(g_renderer->m_font);
			}

			if (ImGui::BeginTabBar("##js_console_tabbar", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Console & REPL"))
				{
					ImGui::TextDisabled("QuickJS Interactive REPL  |  Ctrl+Enter to run");
					ImGui::SameLine();
					const char* snippets[] = {"Console log", "Vector3", "Mono runtime"};
					ImGui::SetNextItemWidth(150);
					if (ImGui::BeginCombo("##js_snippet", snippets[snippet]))
					{
						for (int i = 0; i < IM_ARRAYSIZE(snippets); ++i)
						{
							if (ImGui::Selectable(snippets[i], snippet == i))
							{
								snippet = i;
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SameLine();
					if (ImGui::Button("Insert sample"))
					{
						static constexpr const char* samples[] = {"console.log('Hello from QuickJS');", "const position = new Vector3(1, 2, 3);\nconsole.log(position.x, position.y, position.z);", "console.log('Mono domain available:', mono.get_root_domain() !== 0);"};
						input_code = samples[snippet];
					}
					ImGui::SameLine();
					if (ImGui::Button("Clear editor"))
					{
						input_code.clear();
					}

					const auto editor_height = std::clamp(ImGui::GetContentRegionAvail().y * 0.34f, 120.0f, 240.0f);
					const auto editor_flags  = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize;
					const bool has_mono_font = g_renderer && g_renderer->m_monospace_font;
					if (has_mono_font)
					{
						ImGui::PushFont(g_renderer->m_monospace_font);
					}
					if (ImGui::InputTextMultiline("##js_editor", &input_code, {-FLT_MIN, editor_height}, editor_flags))
					{
						if (auto cmd = commands::get_command<bool_command>(joaat("disable_input")))
						{
							cmd->set_state(true);
						}
					}
					if (ImGui::IsItemActive())
					{
						if (auto cmd = commands::get_command<bool_command>(joaat("disable_input")))
						{
							cmd->set_state(true);
						}
					}
					if (has_mono_font)
					{
						ImGui::PopFont();
					}

					const bool run_requested = ImGui::Button("Run  Ctrl+Enter", {130, 0}) || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Enter));
					if (run_requested)
					{
						auto code = input_code;
						async_run([code = std::move(code)] {
							javascript_manager::eval_script(code);
						});
					}
					ImGui::SameLine();
					ImGui::TextDisabled("Output is captured below");

					ImGui::Separator();
					auto entries = javascript_manager::get_console_entries();
					ImGui::Text("Output  %zu", entries.size());
					ImGui::SameLine();
					ImGui::Checkbox("Info", &show_info);
					ImGui::SameLine();
					ImGui::Checkbox("Warn", &show_warn);
					ImGui::SameLine();
					ImGui::Checkbox("Error", &show_error);
					ImGui::SameLine();
					ImGui::Checkbox("In/Out", &show_eval);
					ImGui::SameLine();
					output_filter.Draw("Filter##output_filter", 140);
					ImGui::SameLine();
					ImGui::Checkbox("Auto-scroll", &auto_scroll);
					ImGui::SameLine();
					if (ImGui::SmallButton("Copy"))
					{
						std::string text;
						for (const auto& entry : entries)
						{
							text += "[" + entry.time + "] " + entry.message + "\n";
						}
						ImGui::SetClipboardText(text.c_str());
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Clear output"))
					{
						javascript_manager::clear_console();
					}
					if (ImGui::BeginChild("##js_output", {0, 0}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
					{
						const bool has_mono_font = g_renderer && g_renderer->m_monospace_font;
						if (has_mono_font)
						{
							ImGui::PushFont(g_renderer->m_monospace_font);
						}
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6, 3});
						if (entries.empty())
						{
							ImGui::TextDisabled("No output yet. Run a snippet or a script to see logs here.");
						}
						for (const auto& entry : entries)
						{
							if (entry.level == js_log_level::info && !show_info)
							{
								continue;
							}
							if (entry.level == js_log_level::warn && !show_warn)
							{
								continue;
							}
							if (entry.level == js_log_level::error && !show_error)
							{
								continue;
							}
							if ((entry.level == js_log_level::eval_in || entry.level == js_log_level::eval_out) && !show_eval)
							{
								continue;
							}
							if (!output_filter.PassFilter(entry.message.c_str()))
							{
								continue;
							}
							// The level label names each entry, so it reads in every theme without its own colour.
							const char* level = "INFO";
							if (entry.level == js_log_level::warn)
							{
								level = "WARN";
							}
							if (entry.level == js_log_level::error)
							{
								level = "ERR ";
							}
							if (entry.level == js_log_level::eval_in)
							{
								level = "IN  ";
							}
							if (entry.level == js_log_level::eval_out)
							{
								level = "OUT ";
							}
							ImGui::TextDisabled("[%s] [%s]", entry.time.c_str(), level);
							ImGui::SameLine();
							ImGui::TextUnformatted(entry.message.c_str());
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("%s", entry.message.c_str());
							}
						}
						ImGui::PopStyleVar();
						if (has_mono_font)
						{
							ImGui::PopFont();
						}
						if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
						{
							ImGui::SetScrollHereY(1.0f);
						}
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Script Manager"))
				{
					if (ImGui::BeginChild("##script_files_pane", {280, 0}, ImGuiChildFlags_Borders))
					{
						ImGui::Text("Installed Scripts  (%zu)", script_files.size());
						script_filter.Draw("Search##scripts", 190);
						if (ImGui::Button("Reload"))
						{
							scan_js_scripts();
						}
						ImGui::SameLine();
						if (ImGui::Button("Open Folder"))
						{
							const auto dir = file_manager::get_project_folder("./scripts");
							ShellExecuteW(nullptr, L"open", dir.get_path().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
						}
						ImGui::Separator();
						for (int i = 0; i < static_cast<int>(script_files.size()); ++i)
						{
							const auto name = script_files[i].filename().string();
							if (!script_filter.PassFilter(name.c_str()))
							{
								continue;
							}
							if (ImGui::Selectable(name.c_str(), selected_script == i))
							{
								selected_script = i;
							}
						}
					}
					ImGui::EndChild();
					ImGui::SameLine();
					if (ImGui::BeginChild("##script_preview", {0, 0}, ImGuiChildFlags_Borders))
					{
						if (selected_script < 0 || selected_script >= static_cast<int>(script_files.size()))
						{
							ImGui::TextDisabled("Choose a script to preview and run it.");
						}
						else
						{
							const auto path          = script_files[selected_script];
							const auto filename      = path.filename().string();
							std::uintmax_t file_size = 0;
							try
							{
								file_size = std::filesystem::file_size(path);
							}
							catch (...)
							{
							}
							ImGui::Text("Script: %s", filename.c_str());
							ImGui::TextDisabled("%s  ·  %.1f KB", path.string().c_str(), file_size / 1024.0);
							ImGui::Spacing();
							if (ImGui::Button("Execute Script"))
							{
								async_run([path] {
									javascript_manager::eval_file(path);
								});
							}
							ImGui::SameLine();
							if (ImGui::Button("Load into REPL"))
							{
								input_code = read_script(path);
							}
							ImGui::SameLine();
							if (ImGui::Button("Open externally"))
							{
								ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
							}
							ImGui::Separator();
							ImGui::TextDisabled("Source Preview (read only)");
							static std::string preview;
							preview = read_script(path);
							if (g_renderer && g_renderer->m_monospace_font)
							{
								ImGui::PushFont(g_renderer->m_monospace_font);
							}
							ImGui::InputTextMultiline("##script_code", &preview, {-FLT_MIN, -FLT_MIN}, ImGuiInputTextFlags_ReadOnly);
							if (g_renderer && g_renderer->m_monospace_font)
							{
								ImGui::PopFont();
							}
						}
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Quick Snippets"))
				{
					ImGui::TextDisabled("Examples for the JavaScript bindings available in this project.");
					const char* titles[] = {"Console output", "Vector3", "Mono runtime"};
					const char* samples[] = {"console.log('Hello from QuickJS');", "const v = new Vector3(12.5, 45.0, -9.8);\nconsole.log(v.x, v.y, v.z);", "console.log('Mono domain available:', mono.get_root_domain() !== 0);"};
					for (int i = 0; i < IM_ARRAYSIZE(titles); ++i)
					{
						if (!ImGui::CollapsingHeader(titles[i], ImGuiTreeNodeFlags_DefaultOpen))
						{
							continue;
						}
						ImGui::PushFont(g_renderer && g_renderer->m_monospace_font ? g_renderer->m_monospace_font : ImGui::GetFont());
						ImGui::TextUnformatted(samples[i]);
						ImGui::PopFont();
						if (ImGui::Button(std::format("Load into REPL##snippet{}", i).c_str()))
						{
							input_code = samples[i];
						}
						ImGui::SameLine();
						if (ImGui::Button(std::format("Execute##snippet{}", i).c_str()))
						{
							const std::string code = samples[i];
							async_run([code] {
								javascript_manager::eval_script(code);
							});
						}
						ImGui::Spacing();
					}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			if (has_ui_font)
			{
				ImGui::PopFont();
			}
		}
		canvas::end_panel();
	}
}
