#include "inventory_editor.hpp"

#include "localization.hpp"
#include "player.hpp"
#include "item_data.hpp"
#include "equipment_slots.hpp"
#include "utility/unity.hpp"
#include "task_mgr.hpp"
#include "notification/notification_service.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <format>
#include <imgui.h>
#include <limits>

namespace big
{
	namespace
	{
		bool s_open = false;
		char s_search[96]{};
		MonoObject* s_selected = nullptr;
		int s_sort_mode        = 0;
		int s_grid_sort_mode   = 0;
		bool s_grid_descending = false;
		int s_inventory_view   = 0;

		std::string inventory_item_name(MonoObject* item)
		{
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
			auto raw = shared ? mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_name", MonoString*>(shared) : nullptr;
			if (!raw)
			{
				return "Unknown item";
			}
			return localization::get_instance().localize(mono::from_mono_string(raw));
		}

		std::string inventory_item_id(MonoObject* item)
		{
			auto prefab = mono::get_field_value<"ItemDrop/ItemData", "m_dropPrefab", MonoObject*>(item);
			return prefab ? unity::get_name(prefab) : "";
		}

		void refresh_inventory(MonoObject* inventory)
		{
			static auto method = []() {
				auto m = mono::get_method("Inventory", "Changed", 2, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method("Inventory", "Changed", 0, "assembly_valheim");
				}
				return m;
			}();
			if (method && inventory)
			{
				if (mono::get_param_count(method) == 2)
				{
					bool success  = true;
					bool cheated  = false;
					void* args[2] = {&success, &cheated};
					mono::invoke_method(method, inventory, args);
				}
				else
				{
					mono::invoke_method(method, inventory, nullptr);
				}
				auto& equipment = equipment_manager::get();
				if (equipment.get_equipment_inventory() == inventory)
				{
					equipment.save_to_player();
				}
			}
		}

		bool can_merge_stack(MonoObject* target, MonoObject* source)
		{
			if (!target || !source || target == source || mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(target) || mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(source))
			{
				return false;
			}

			auto target_shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(target);
			auto source_shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(source);
			if (!target_shared || target_shared != source_shared)
			{
				return false;
			}

			const int max_stack = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxStackSize", int>(target_shared);
			if (max_stack <= 1 || item_data(target).get_stack() >= max_stack)
			{
				return false;
			}
			if (item_data(target).get_quality() != item_data(source).get_quality()
			    || item_data(target).get_variant() != item_data(source).get_variant() || mono::get_field_value<"ItemDrop/ItemData", "m_worldLevel", uint8_t>(target) != mono::get_field_value<"ItemDrop/ItemData", "m_worldLevel", uint8_t>(source) || mono::get_field_value<"ItemDrop/ItemData", "m_crafterID", int64_t>(target) != mono::get_field_value<"ItemDrop/ItemData", "m_crafterID", int64_t>(source)
			    || item_data(target).get_crafter_name() != item_data(source).get_crafter_name())
			{
				return false;
			}

			static auto custom_data_field = mono::get_field("ItemDrop/ItemData", "m_customData", "assembly_valheim");
			if (!custom_data_field)
			{
				return false;
			}
			auto has_custom_data = [](MonoObject* item) {
				MonoObject* data = nullptr;
				mono::get_field_value(item, custom_data_field, &data);
				if (!data)
				{
					return false;
				}
				auto klass = mono::object_get_class(data);
				auto count = klass ? mono::class_get_method_from_name(klass, "get_Count", 0) : nullptr;
				auto value = count ? mono::invoke_method(count, data, nullptr) : nullptr;
				return !value || !mono::object_unbox(value) || *static_cast<int*>(mono::object_unbox(value)) > 0;
			};
			if (has_custom_data(target) || has_custom_data(source))
			{
				return false;
			}

			const float target_max_durability = item_data(target).get_max_durability();
			const float source_max_durability = item_data(source).get_max_durability();
			if (target_max_durability > 0.f || source_max_durability > 0.f)
			{
				if (std::abs(target_max_durability - source_max_durability) > 0.01f
				    || std::abs(item_data(target).get_durability() - item_data(source).get_durability()) > 0.01f)
				{
					return false;
				}
			}
			return true;
		}

		void consolidate_inventory(MonoObject* inventory)
		{
			static auto get_all = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
			static auto remove_item = mono::get_method_exact("Inventory", "RemoveItem", {"ItemDrop/ItemData"}, "assembly_valheim");
			if (!inventory || !get_all || !remove_item)
			{
				notification::warning("Inventory Editor", "Inventory stack methods are unavailable.");
				return;
			}

			auto list        = mono::invoke_method(get_all, inventory, nullptr);
			auto items       = unity::list_to_vector(list);
			int merged_units = 0;
			for (std::size_t source_index = 0; source_index < items.size(); ++source_index)
			{
				auto* source = items[source_index];
				if (!source || mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(source))
				{
					continue;
				}
				for (std::size_t target_index = 0; target_index < source_index; ++target_index)
				{
					auto* target = items[target_index];
					if (!can_merge_stack(target, source))
					{
						continue;
					}
					const int max_stack = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxStackSize", int>(mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(target));
					const int target_stack = item_data(target).get_stack();
					const int source_stack = item_data(source).get_stack();
					const int transfer     = std::min(std::max(0, max_stack - target_stack), source_stack);
					if (transfer <= 0)
					{
						continue;
					}
					item_data(target).set_stack(target_stack + transfer);
					item_data(source).set_stack(source_stack - transfer);
					merged_units += transfer;
					if (source_stack == transfer)
					{
						void* args[] = {source};
						mono::invoke_method(remove_item, inventory, args);
						items[source_index] = nullptr;
						break;
					}
				}
			}
			refresh_inventory(inventory);
			if (merged_units > 0)
			{
				notification::success("Inventory Editor", std::format("Consolidated {} item units into existing stacks.", merged_units));
			}
			else
			{
				notification::info("Inventory Editor", "No compatible stacks could be consolidated.");
			}
		}
	}

	void inventory_editor::toggle_window()
	{
		s_open = !s_open;
	}

	void inventory_editor::draw_window()
	{
		if (s_open)
		{
			draw_window_impl();
		}
	}

	void inventory_editor::draw_window_impl()
	{
		ImGui::SetNextWindowSize({620.f, 560.f}, ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Inventory Editor", &s_open))
		{
			ImGui::End();
			return;
		}

		auto player_obj = unity::get_local_player();
		if (!player_obj)
		{
			ImGui::TextDisabled("Join a world to inspect your inventory.");
			ImGui::End();
			return;
		}

		player local_player(player_obj);
		auto inventory  = local_player.get_inventory();
		auto main_inv   = inventory.get_object();
		auto& equipment = equipment_manager::get();
		equipment.init_or_sync();
		auto equipment_inv  = equipment.get_equipment_inventory();
		static auto get_all = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");

		const char* inventory_views[] = {"Backpack", "Equipment Slots"};
		if (equipment_inv && equipment_inv != main_inv)
		{
			ImGui::SetNextItemWidth(190.f);
			ImGui::Combo("Storage", &s_inventory_view, inventory_views, IM_ARRAYSIZE(inventory_views));
		}
		else
		{
			s_inventory_view = 0;
			ImGui::TextDisabled("Storage: Backpack");
		}
		auto selected_inventory = s_inventory_view == 1 && equipment_inv ? equipment_inv : main_inv;
		auto items_obj = get_all && selected_inventory ? mono::invoke_method(get_all, selected_inventory, nullptr) : nullptr;
		auto items = unity::list_to_vector(items_obj);
		items.erase(std::remove(items.begin(), items.end(), nullptr), items.end());

		ImGui::SameLine();
		ImGui::Text("Items: %zu", items.size());
		ImGui::SameLine();
		if (ImGui::SmallButton("Repair All"))
		{
			auto inv_obj       = main_inv;
			auto equipment_obj = equipment_inv;
			auto item_copy = get_all && inv_obj ? unity::list_to_vector(mono::invoke_method(get_all, inv_obj, nullptr)) : std::vector<MonoObject*>{};
			if (equipment_obj && equipment_obj != inv_obj && get_all)
			{
				auto equipment_items = unity::list_to_vector(mono::invoke_method(get_all, equipment_obj, nullptr));
				for (auto* item : equipment_items)
				{
					if (item && std::find(item_copy.begin(), item_copy.end(), item) == item_copy.end())
					{
						item_copy.push_back(item);
					}
				}
			}
			async_run([item_copy, inv_obj, equipment_obj] {
				for (auto* item : item_copy)
				{
					item_data data(item);
					data.set_durability(data.get_max_durability());
				}
				refresh_inventory(inv_obj);
				if (equipment_obj && equipment_obj != inv_obj)
				{
					refresh_inventory(equipment_obj);
				}
			});
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Consolidate Stacks"))
		{
			auto inv_obj = selected_inventory;
			async_run([inv_obj] {
				consolidate_inventory(inv_obj);
			});
		}

		const char* sort_modes[] = {"Name", "Stack size", "Quality", "Durability", "Grid position"};
		ImGui::SetNextItemWidth(160.f);
		ImGui::Combo("Display sorting", &s_sort_mode, sort_modes, IM_ARRAYSIZE(sort_modes));
		std::stable_sort(items.begin(), items.end(), [](MonoObject* left, MonoObject* right) {
			switch (s_sort_mode)
			{
			case 1: return item_data(left).get_stack() < item_data(right).get_stack();
			case 2: return item_data(left).get_quality() < item_data(right).get_quality();
			case 3: return item_data(left).get_durability() < item_data(right).get_durability();
			case 4:
			{
				const auto left_pos  = item_data(left).get_grid_pos();
				const auto right_pos = item_data(right).get_grid_pos();
				return left_pos.y == right_pos.y ? left_pos.x < right_pos.x : left_pos.y < right_pos.y;
			}
			default: return inventory_item_name(left) < inventory_item_name(right);
			}
		});
		const char* grid_sort_modes[] = {"Name", "Stack size", "Quality", "Durability"};
		ImGui::SetNextItemWidth(160.f);
		ImGui::Combo("Grid sorting", &s_grid_sort_mode, grid_sort_modes, IM_ARRAYSIZE(grid_sort_modes));
		ImGui::SameLine();
		ImGui::Checkbox("Descending", &s_grid_descending);
		ImGui::SameLine();
		if (s_inventory_view == 0 && ImGui::Button("Arrange Grid"))
		{
			auto inventory_object = selected_inventory;
			auto grid_items       = items;
			const int sort_mode   = s_grid_sort_mode;
			const bool descending = s_grid_descending;
			async_run([inventory_object, grid_items = std::move(grid_items), sort_mode, descending]() mutable {
				static auto width_field    = mono::get_field("Inventory", "m_width", "assembly_valheim");
				static auto grid_pos_field = mono::get_field("ItemDrop/ItemData", "m_gridPos", "assembly_valheim");
				if (!inventory_object || !width_field || !grid_pos_field)
				{
					notification::warning("Inventory Editor", "Inventory grid metadata is unavailable.");
					return;
				}
				int width = 0;
				mono::get_field_value(inventory_object, width_field, &width);
				if (width <= 0 || grid_items.empty())
				{
					return;
				}
				std::stable_sort(grid_items.begin(), grid_items.end(), [sort_mode, descending](MonoObject* left, MonoObject* right) {
					switch (sort_mode)
					{
					case 1:
						return descending ? item_data(left).get_stack() > item_data(right).get_stack() :
						                    item_data(left).get_stack() < item_data(right).get_stack();
					case 2:
						return descending ? item_data(left).get_quality() > item_data(right).get_quality() :
						                    item_data(left).get_quality() < item_data(right).get_quality();
					case 3:
						return descending ? item_data(left).get_durability() > item_data(right).get_durability() :
						                    item_data(left).get_durability() < item_data(right).get_durability();
					default:
						return descending ? inventory_item_name(left) > inventory_item_name(right) : inventory_item_name(left) < inventory_item_name(right);
					}
				});
				for (std::size_t i = 0; i < grid_items.size(); ++i)
				{
					iVector2 position{static_cast<int>(i % static_cast<std::size_t>(width)), static_cast<int>(i / static_cast<std::size_t>(width))};
					mono::set_field_value(grid_items[i], grid_pos_field, &position);
				}
				refresh_inventory(inventory_object);
				notification::success("Inventory Editor", "Inventory grid arranged using the selected sort order.");
			});
		}
		else if (s_inventory_view == 1)
		{
			ImGui::TextDisabled("Dedicated equipment slot positions are preserved.");
		}
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##InventorySearch", "Search item name or prefab ID", s_search, sizeof(s_search));
		std::string query = s_search;
		std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});

		ImGui::BeginChild("InventoryItems", {260.f, -1.f}, true);
		for (auto* item : items)
		{
			auto name          = inventory_item_name(item);
			auto prefab        = inventory_item_id(item);
			std::string needle = name + " " + prefab;
			std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			if (!query.empty() && needle.find(query) == std::string::npos)
			{
				continue;
			}

			item_data data(item);
			std::string label = std::format("{}  x{}  Q{}", name, data.get_stack(), data.get_quality());
			ImGui::PushID(item);
			if (ImGui::Selectable(label.c_str(), s_selected == item))
			{
				s_selected = item;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::BeginChild("InventoryItemDetails", {0.f, -1.f}, false);
		if (s_selected && std::find(items.begin(), items.end(), s_selected) != items.end())
		{
			item_data data(s_selected);
			auto name = inventory_item_name(s_selected);
			ImGui::TextWrapped("%s", name.c_str());
			ImGui::TextDisabled("%s", inventory_item_id(s_selected).c_str());
			ImGui::Separator();

			static int quality            = 1;
			static int variant            = 0;
			static int stack              = 1;
			static int drop_amount        = 1;
			static int durability_percent = 100;
			static bool override_enabled[8]{};
			static float override_values[8]{1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
			static MonoObject* values_item = nullptr;
			if (values_item != s_selected)
			{
				values_item = s_selected;
				quality     = data.get_quality();
				variant     = data.get_variant();
				stack       = data.get_stack();
				drop_amount = data.get_stack();
				durability_percent =
				    data.get_max_durability() > 0.f ? static_cast<int>(std::round(100.f * data.get_durability() / data.get_max_durability())) : 100;
				const char* override_keys[] = {"ew_damage_multiplier", "ew_armor", "ew_block", "ew_parry", "ew_knockback", "ew_backstab", "ew_weight", "ew_max_durability"};
				for (int i = 0; i < IM_ARRAYSIZE(override_keys); ++i)
				{
					override_enabled[i] = data.get_stat_override(override_keys[i], override_values[i]);
				}
				override_values[0] = override_enabled[0] ? override_values[0] : 1.f;
			}

			auto shared_obj = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(s_selected);
			int max_quality = shared_obj ? std::max(1, mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxQuality", int>(shared_obj)) : 1;
			ImGui::SetNextItemWidth(-1.f);
			ImGui::SliderInt("Quality", &quality, 1, std::max(1, max_quality));
			ImGui::SetNextItemWidth(-1.f);
			ImGui::SliderInt("Variant", &variant, 0, 20);
			ImGui::SetNextItemWidth(-1.f);
			ImGui::SliderInt("Stack", &stack, 1, 10000);
			ImGui::SetNextItemWidth(-1.f);
			ImGui::SliderInt("Durability %", &durability_percent, 0, 100);
			if (ImGui::CollapsingHeader("Instance stat overrides"))
			{
				const char* override_labels[] = {"Damage multiplier", "Armor", "Block", "Parry force", "Knockback", "Backstab", "Weight per item", "Maximum durability"};
				const float mins[] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
				const float maxs[] = {100.f, 10000.f, 10000.f, 10000.f, 10000.f, 100.f, 1000.f, 100000.f};
				for (int i = 0; i < IM_ARRAYSIZE(override_labels); ++i)
				{
					ImGui::PushID(i);
					ImGui::Checkbox(override_labels[i], &override_enabled[i]);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(110.f);
					ImGui::BeginDisabled(!override_enabled[i]);
					ImGui::InputFloat("##value", &override_values[i], 0.f, 0.f, "%.2f");
					ImGui::EndDisabled();
					override_values[i] = std::clamp(override_values[i], mins[i], maxs[i]);
					ImGui::PopID();
				}
			}

			if (ImGui::Button("Apply Stats", {-1.f, 0.f}))
			{
				auto* selected             = s_selected;
				auto inv_obj               = selected_inventory;
				int new_quality            = quality;
				int new_variant            = variant;
				int new_stack              = stack;
				int new_durability_percent = durability_percent;
				std::array<bool, 8> new_override_enabled{};
				std::array<float, 8> new_override_values{};
				std::copy(std::begin(override_enabled), std::end(override_enabled), new_override_enabled.begin());
				std::copy(std::begin(override_values), std::end(override_values), new_override_values.begin());
				async_run([selected, inv_obj, new_quality, new_variant, new_stack, new_durability_percent, new_override_enabled, new_override_values] {
					item_data edit(selected);
					edit.set_quality(new_quality);
					edit.set_variant(new_variant);
					edit.set_stack(new_stack);
					const char* override_keys[] = {"ew_damage_multiplier", "ew_armor", "ew_block", "ew_parry", "ew_knockback", "ew_backstab", "ew_weight", "ew_max_durability"};
					for (int i = 0; i < new_override_enabled.size(); ++i)
					{
						if (new_override_enabled[i])
						{
							edit.set_stat_override(override_keys[i], new_override_values[i]);
						}
						else
						{
							edit.remove_stat_override(override_keys[i]);
						}
					}
					edit.set_durability(edit.get_max_durability() * new_durability_percent / 100.f);
					refresh_inventory(inv_obj);
				});
			}
			if (ImGui::Button("Repair Item", {-1.f, 0.f}))
			{
				auto* selected = s_selected;
				auto inv_obj   = selected_inventory;
				async_run([selected, inv_obj] {
					item_data edit(selected);
					edit.set_durability(edit.get_max_durability());
					refresh_inventory(inv_obj);
				});
			}
			if (ImGui::Button("Duplicate Item", {-1.f, 0.f}))
			{
				auto* selected = s_selected;
				auto inv_obj   = selected_inventory;
				async_run([selected, inv_obj] {
					static auto clone        = mono::get_method("ItemDrop/ItemData", "Clone", 0, "assembly_valheim");
					static auto add          = mono::get_method("Inventory", "AddItem", 1, "assembly_valheim");
					static auto get_all      = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
					static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
					static auto custom_data_field = mono::get_field("ItemDrop/ItemData", "m_customData", "assembly_valheim");
					if (!selected || !inv_obj || !clone || !add || !get_all || !shared_field || !custom_data_field)
					{
						notification::warning("Inventory Editor", "Item duplication methods or item metadata are unavailable.");
						return;
					}
					auto copy = mono::invoke_method(clone, selected, nullptr);
					if (!copy)
					{
						return;
					}
					mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(copy, false);

					auto has_custom_data = [&](MonoObject* item) {
						MonoObject* data = nullptr;
						mono::get_field_value(item, custom_data_field, &data);
						if (!data)
						{
							return true;
						}
						auto data_class = mono::object_get_class(data);
						auto count = data_class ? mono::class_get_method_from_name(data_class, "get_Count", 0) : nullptr;
						auto result = count ? mono::invoke_method(count, data, nullptr) : nullptr;
						return !result || *static_cast<int*>(mono::object_unbox(result)) > 0;
					};

					MonoObject* selected_shared = nullptr;
					mono::get_field_value(selected, shared_field, &selected_shared);
					const auto selected_world_level = mono::get_field_value<"ItemDrop/ItemData", "m_worldLevel", uint8_t>(selected);
					const bool selected_has_custom_data = has_custom_data(selected);
					auto all_items                      = mono::invoke_method(get_all, inv_obj, nullptr);
					std::vector<std::pair<MonoObject*, int>> isolated;
					for (auto* item : unity::list_to_vector(all_items))
					{
						if (!item)
						{
							continue;
						}
						MonoObject* item_shared = nullptr;
						mono::get_field_value(item, shared_field, &item_shared);
						if (item_shared != selected_shared || item_data(item).get_quality() != item_data(selected).get_quality() || mono::get_field_value<"ItemDrop/ItemData", "m_worldLevel", uint8_t>(item) != selected_world_level)
						{
							continue;
						}

						const bool same_instance_metadata = !selected_has_custom_data && !has_custom_data(item) && !mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(item)
						    && item_data(item).get_variant() == item_data(selected).get_variant()
						    && std::abs(item_data(item).get_durability() - item_data(selected).get_durability()) < 0.01f
						    && item_data(item).get_cheated() == item_data(selected).get_cheated()
						    && item_data(item).get_crafter_id() == item_data(selected).get_crafter_id()
						    && item_data(item).get_crafter_name() == item_data(selected).get_crafter_name();
						if (same_instance_metadata)
						{
							continue;
						}

						const int quality = item_data(item).get_quality();
						if (mono::set_field_value<"ItemDrop/ItemData", "m_quality">(item, 0))
						{
							isolated.emplace_back(item, quality);
						}
					}

					void* args[1]        = {copy};
					auto added           = mono::invoke_method(add, inv_obj, args);
					const bool succeeded = added && *static_cast<bool*>(mono::object_unbox(added));
					for (const auto& [item, quality] : isolated)
					{
						mono::set_field_value<"ItemDrop/ItemData", "m_quality">(item, quality);
					}
					refresh_inventory(inv_obj);
					if (succeeded)
					{
						notification::success("Inventory Editor", "Item duplicated.");
					}
					else
					{
						notification::warning("Inventory Editor", "No free slot was available for this duplicate.");
					}
				});
			}
			ImGui::SetNextItemWidth(-1.f);
			ImGui::SliderInt("Drop quantity", &drop_amount, 1, std::max(1, data.get_stack()));
			if (ImGui::Button("Drop Selected", {-1.f, 0.f}))
			{
				auto* selected       = s_selected;
				auto* player_object  = player_obj;
				auto inv_obj         = selected_inventory;
				int amount           = std::clamp(drop_amount, 1, std::max(1, data.get_stack()));
				Vector3 position     = unity::get_position(player_obj) + unity::get_forward(player_obj) * 1.5f;
				Quaternions rotation = unity::get_rotation(player_obj);
				async_run([selected, player_object, inv_obj, amount, position, rotation]() mutable {
					if (unity::get_local_player() != player_object || !selected || !inv_obj)
					{
						return;
					}
					static auto drop_item    = mono::get_method("ItemDrop", "DropItem", 4, "assembly_valheim");
					static auto remove_item  = mono::get_method("Inventory", "RemoveItem", 2, "assembly_valheim");
					static auto unequip_item = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
					static auto equip_item   = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
					if (!drop_item || !remove_item)
					{
						notification::warning("Inventory Editor", "Drop or remove item method is unavailable.");
						return;
					}
					const bool remove_entire_stack = amount >= item_data(selected).get_stack();
					const bool was_equipped = remove_entire_stack && mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(selected);
					bool trigger = true;
					if (was_equipped)
					{
						if (!unequip_item)
						{
							notification::warning("Inventory Editor", "Humanoid.UnequipItem is unavailable; equipped item was not dropped.");
							return;
						}
						void* unequip_args[2] = {selected, &trigger};
						mono::invoke_method(unequip_item, player_object, unequip_args);
					}
					int drop_count     = amount;
					void* drop_args[4] = {selected, &drop_count, &position, &rotation};
					auto dropped       = mono::invoke_method(drop_item, nullptr, drop_args);
					if (dropped)
					{
						auto dropped_item_data = mono::get_field_value<"ItemDrop", "m_itemData", MonoObject*>(dropped);
						if (dropped_item_data)
						{
							mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(dropped_item_data, false);
						}
						void* remove_args[2] = {selected, &drop_count};
						auto removed         = mono::invoke_method(remove_item, inv_obj, remove_args);
						if (removed && *reinterpret_cast<bool*>(mono::object_unbox(removed)))
						{
							refresh_inventory(inv_obj);
						}
						else if (was_equipped && equip_item)
						{
							void* equip_args[2] = {selected, &trigger};
							mono::invoke_method(equip_item, player_object, equip_args);
						}
					}
					else if (was_equipped && equip_item)
					{
						void* equip_args[2] = {selected, &trigger};
						mono::invoke_method(equip_item, player_object, equip_args);
					}
				});
				if (amount >= data.get_stack())
				{
					s_selected = nullptr;
				}
			}
			bool equipped = mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(s_selected);
			if (ImGui::Button(equipped ? "Unequip" : "Equip", {-1.f, 0.f}))
			{
				auto* selected = s_selected;
				async_run([player_obj, selected, equipped] {
					static auto equip   = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
					static auto unequip = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
					bool trigger        = true;
					void* args[2]       = {selected, &trigger};
					if (equipped && unequip)
					{
						mono::invoke_method(unequip, player_obj, args);
					}
					else if (!equipped && equip)
					{
						mono::invoke_method(equip, player_obj, args);
					}
				});
			}
			if (ImGui::Button("Delete Item", {-1.f, 0.f}))
			{
				auto* selected      = s_selected;
				auto* player_object = player_obj;
				auto inv_obj        = selected_inventory;
				async_run([selected, player_object, inv_obj] {
					if (unity::get_local_player() != player_object || !selected || !inv_obj)
					{
						return;
					}
					static auto remove = mono::get_method("Inventory", "RemoveItem", 2, "assembly_valheim");
					if (!remove)
					{
						return;
					}
					const bool was_equipped = mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(selected);
					static auto unequip     = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
					static auto equip       = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
					bool trigger            = true;
					if (was_equipped)
					{
						if (!unequip)
						{
							notification::warning("Inventory Editor", "Humanoid.UnequipItem is unavailable; equipped item was not deleted.");
							return;
						}
						void* unequip_args[2] = {selected, &trigger};
						mono::invoke_method(unequip, player_object, unequip_args);
					}
					int amount    = item_data(selected).get_stack();
					void* args[2] = {selected, &amount};
					auto removed  = mono::invoke_method(remove, inv_obj, args);
					if (removed && *reinterpret_cast<bool*>(mono::object_unbox(removed)))
					{
						refresh_inventory(inv_obj);
					}
					else if (was_equipped && equip)
					{
						void* equip_args[2] = {selected, &trigger};
						mono::invoke_method(equip, player_object, equip_args);
					}
				});
				s_selected = nullptr;
			}
		}
		else
		{
			ImGui::TextDisabled("Select an item to edit.");
		}
		ImGui::EndChild();
		ImGui::End();
	}
}
