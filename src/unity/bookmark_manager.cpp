#include "bookmark_manager.hpp"

#include <astra/host/notification.hpp>
#include "task_mgr.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <astra/host/canvas.hpp>
#include <imgui.h>
#include <nlohmann/json.hpp>

namespace big
{
	namespace
	{
		using json = nlohmann::json;
		struct bookmark
		{
			std::string name;
			Vector3 position{};
		};

		bool s_open = false;
		char s_name[80]{};

		std::string world_bookmark_key()
		{
			auto znet_class  = mono::get_class("ZNet", "assembly_valheim");
			auto world_field = znet_class ? mono::get_field(znet_class, "m_world") : nullptr;
			auto static_data = znet_class ? mono::get_static_field_data(znet_class) : nullptr;
			if (!world_field || !static_data)
			{
				return {};
			}
			auto world = *reinterpret_cast<MonoObject**>(reinterpret_cast<uintptr_t>(static_data) + mono::get_field_offset(world_field));
			if (!world)
			{
				return {};
			}
			auto uid = mono::get_field_value<"World", "m_uid", int64_t>(world);
			return "ew_bookmarks_" + std::to_string(uid);
		}

		MonoObject* get_custom_data(MonoObject* player)
		{
			return player ? mono::get_field_value<"Player", "m_customData", MonoObject*>(player) : nullptr;
		}

		std::string read_custom_data(MonoObject* dictionary, const std::string& key)
		{
			if (!dictionary)
			{
				return {};
			}
			auto klass                       = mono::object_get_class(dictionary);
			static MonoMethod* try_get_value = nullptr;
			if (!try_get_value)
			{
				try_get_value = klass ? mono::class_get_method_from_name(klass, "TryGetValue", 2) : nullptr;
			}
			if (!try_get_value)
			{
				return {};
			}
			MonoString* output = nullptr;
			void* args[2]      = {mono::to_mono_string(key), &output};
			auto result        = mono::invoke_method(try_get_value, dictionary, args);
			if (!result || !*reinterpret_cast<bool*>(mono::object_unbox(result)) || !output)
			{
				return {};
			}
			return mono::from_mono_string(output);
		}

		bool write_custom_data(MonoObject* dictionary, const std::string& key, const std::string& value)
		{
			if (!dictionary)
			{
				return false;
			}
			auto klass                  = mono::object_get_class(dictionary);
			static MonoMethod* set_item = nullptr;
			if (!set_item)
			{
				set_item = klass ? mono::class_get_method_from_name(klass, "set_Item", 2) : nullptr;
			}
			if (!set_item)
			{
				return false;
			}
			void* args[2] = {mono::to_mono_string(key), mono::to_mono_string(value)};
			mono::invoke_method(set_item, dictionary, args);
			return true;
		}

		std::vector<bookmark> load_bookmarks()
		{
			auto player = unity::get_local_player();
			auto key    = world_bookmark_key();
			if (!player || key.empty())
			{
				return {};
			}
			auto raw = read_custom_data(get_custom_data(player), key);
			if (raw.empty())
			{
				return {};
			}
			try
			{
				auto data = json::parse(raw);
				std::vector<bookmark> result;
				for (const auto& item : data)
				{
					result.push_back({item.value("name", std::string{}), {item.value("x", 0.f), item.value("y", 0.f), item.value("z", 0.f)}});
				}
				return result;
			}
			catch (...)
			{
				return {};
			}
		}

		bool save_bookmarks(const std::vector<bookmark>& bookmarks)
		{
			auto player     = unity::get_local_player();
			auto key        = world_bookmark_key();
			auto dictionary = get_custom_data(player);
			if (!player || !dictionary || key.empty())
			{
				return false;
			}
			json data = json::array();
			for (const auto& item : bookmarks)
			{
				data.push_back({{"name", item.name}, {"x", item.position.x}, {"y", item.position.y}, {"z", item.position.z}});
			}
			return write_custom_data(dictionary, key, data.dump());
		}
	}

	void bookmark_manager::toggle_window()
	{
		s_open = !s_open;
	}

	void bookmark_manager::draw_window()
	{
		if (!s_open)
		{
			return;
		}
		if (!canvas::begin_panel("World Bookmarks", &s_open, {.size = {520.f, 460.f}}))
		{
			canvas::end_panel();
			return;
		}
		auto player = unity::get_local_player();
		if (!player || world_bookmark_key().empty())
		{
			ImGui::TextDisabled("Join a world to create bookmarks.");
			canvas::end_panel();
			return;
		}
		ImGui::TextDisabled("Per-world coordinates  /  saved with this character");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##BookmarkName", "Bookmark name", s_name, sizeof(s_name));
		if (ImGui::Button("Save Current Location", {-1.f, 0.f}) && s_name[0])
		{
			auto items       = load_bookmarks();
			std::string name = s_name;
			auto position    = unity::get_position(player);
			auto found       = std::find_if(items.begin(), items.end(), [&](const bookmark& item) {
				return item.name == name;
			});
			if (found == items.end())
			{
				items.push_back({name, position});
			}
			else
			{
				found->position = position;
			}
			if (save_bookmarks(items))
			{
				notification::success("World Bookmarks",
				    std::format("Saved {} at {:.1f}, {:.1f}, {:.1f}.", name, position.x, position.y, position.z));
			}
			s_name[0] = '\0';
		}
		ImGui::Separator();
		auto bookmarks = load_bookmarks();
		ImGui::Text("Saved locations  /  %zu", bookmarks.size());
		ImGui::BeginChild("BookmarkList", {0.f, -1.f}, true);
		for (std::size_t i = 0; i < bookmarks.size(); ++i)
		{
			const auto item = bookmarks[i];
			ImGui::PushID(static_cast<int>(i));
			ImGui::TextUnformatted(item.name.c_str());
			ImGui::TextDisabled("X %.1f   Y %.1f   Z %.1f", item.position.x, item.position.y, item.position.z);
			if (ImGui::SmallButton("Teleport"))
			{
				auto position = item.position;
				async_run([position] {
					static auto teleport = mono::get_method("Player", "TeleportTo", 3, "assembly_valheim");
					auto player_obj      = unity::get_local_player();
					if (!teleport || !player_obj)
					{
						return;
					}
					auto target_position = position;
					Quaternions rotation = unity::get_rotation(player_obj);
					bool distant         = true;
					void* args[3]        = {&target_position, &rotation, &distant};
					mono::invoke_method(teleport, player_obj, args);
				});
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete"))
			{
				bookmarks.erase(bookmarks.begin() + static_cast<std::ptrdiff_t>(i));
				save_bookmarks(bookmarks);
			}
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::EndChild();
		canvas::end_panel();
	}
}
