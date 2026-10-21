#include "equipment_slots.hpp"
#include "hooking.hpp"
#include "menu_settings.hpp"
#include <astra/host/notification.hpp>
#include "unity/item_icons.hpp"
#include "unity/localization.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <astra/host/canvas.hpp>
#include <format>
#include <unordered_set>

namespace big
{
	namespace
	{
		struct unity_color
		{
			float r;
			float g;
			float b;
			float a;
		};

		// 8 columns x 5 rows grid layout according to BetrValheim v4:
		// Row 0: H  U1 U2 .  .  K1 F1 A1
		// Row 1: P  U3 U4 .  .  K2 F2 A2
		// Row 2: C  T  .  .  .  .  F3 A3
		// Row 3: L  .  .  .  .  .  .  .
		// Row 4: .  .  Q1 Q2 Q3 Q4 Q5 Q6
		const slot_kind s_layout[equipment_manager::grid_height][equipment_manager::grid_width] = {{slot_kind::helmet, slot_kind::utility, slot_kind::utility, slot_kind::none, slot_kind::none, slot_kind::key, slot_kind::food, slot_kind::ammo}, {slot_kind::cape, slot_kind::utility, slot_kind::utility, slot_kind::none, slot_kind::none, slot_kind::key, slot_kind::food, slot_kind::ammo}, {slot_kind::chest, slot_kind::trinket, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::food, slot_kind::ammo}, {slot_kind::legs, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none}, {slot_kind::none, slot_kind::none, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick}};

		std::string get_item_display_name(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return "";
			}
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item_obj);
			if (!shared)
			{
				return "Unknown Item";
			}
			auto raw_mono = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_name", MonoString*>(shared);
			if (!raw_mono)
			{
				return "Unknown Item";
			}
			std::string raw_name = mono::from_mono_string(raw_mono);
			if (raw_name.empty())
			{
				return "Unknown Item";
			}
			return localization::get_instance().localize(raw_name);
		}

		std::string get_item_prefab_name(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return "";
			}
			auto drop_prefab = mono::get_field_value<"ItemDrop/ItemData", "m_dropPrefab", MonoObject*>(item_obj);
			return drop_prefab ? unity::get_name(drop_prefab) : "";
		}

		float get_item_durability(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return 0.f;
			}
			return mono::get_field_value<"ItemDrop/ItemData", "m_durability", float>(item_obj);
		}

		float get_item_max_durability(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return 1.f;
			}
			static auto get_max_dur = mono::get_method("ItemDrop/ItemData", "GetMaxDurability", 0, "assembly_valheim");
			if (!get_max_dur)
			{
				return 100.f;
			}
			auto res = mono::invoke_method(get_max_dur, item_obj, nullptr);
			return res ? *reinterpret_cast<float*>(mono::object_unbox(res)) : 100.f;
		}

		int get_item_stack(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return 0;
			}
			return mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item_obj);
		}

		bool is_item_equipped(MonoObject* item_obj)
		{
			if (!item_obj)
			{
				return false;
			}
			return mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(item_obj);
		}
	}

	slot_kind equipment_manager::get_slot_kind(int x, int y)
	{
		if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
		{
			return slot_kind::none;
		}
		return s_layout[y][x];
	}

	const char* equipment_manager::get_slot_kind_name(slot_kind kind)
	{
		switch (kind)
		{
		case slot_kind::helmet: return "Helmet";
		case slot_kind::cape: return "Cape";
		case slot_kind::chest: return "Chest";
		case slot_kind::legs: return "Legs";
		case slot_kind::trinket: return "Trinket";
		case slot_kind::utility: return "Utility";
		case slot_kind::food: return "Food";
		case slot_kind::ammo: return "Ammo";
		case slot_kind::key: return "Key";
		case slot_kind::quick: return "Quick Slot";
		default: return "";
		}
	}

	int equipment_manager::get_slot_number(slot_kind kind, int x, int y)
	{
		if (kind == slot_kind::none)
		{
			return 0;
		}
		int count = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (s_layout[r][c] == kind)
				{
					count++;
					if (c == x && r == y)
					{
						return count;
					}
				}
			}
		}
		return 0;
	}

	iVector2 equipment_manager::get_slot_pos(slot_kind kind, int number)
	{
		if (kind == slot_kind::none || number <= 0)
		{
			return {-1, -1};
		}
		int count = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (s_layout[r][c] == kind)
				{
					count++;
					if (count == number)
					{
						return {c, r};
					}
				}
			}
		}
		return {-1, -1};
	}

	bool equipment_manager::fits_slot(slot_kind kind, MonoObject* item_data_obj)
	{
		if (!item_data_obj || kind == slot_kind::none)
		{
			return false;
		}
		if (kind == slot_kind::quick)
		{
			return true;
		}

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item_data_obj);
		if (!shared)
		{
			return false;
		}

		int item_type = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared);

		switch (kind)
		{
		case slot_kind::helmet: return item_type == 6;
		case slot_kind::chest: return item_type == 7;
		case slot_kind::legs: return item_type == 11;
		case slot_kind::cape: return item_type == 17;
		case slot_kind::trinket: return item_type == 24;
		case slot_kind::utility: return item_type == 18;
		case slot_kind::food:
			if (item_type == 2)
			{
				float food = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_food", float>(shared);
				float stam = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_foodStamina", float>(shared);
				float eitr = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_foodEitr", float>(shared);
				return (food > 0.f || stam > 0.f || eitr > 0.f);
			}
			return false;
		case slot_kind::ammo: return item_type == 9;
		case slot_kind::key:
		{
			bool quest = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_questItem", bool>(shared);
			return quest || item_type == 16;
		}
		default: return false;
		}
	}

	bool equipment_manager::fits_slot(iVector2 pos, MonoObject* item_data_obj)
	{
		return fits_slot(get_slot_kind(pos.x, pos.y), item_data_obj);
	}

	iVector2 equipment_manager::find_home_slot(MonoObject* equipment_inv, MonoObject* item_data_obj)
	{
		if (!equipment_inv || !item_data_obj)
		{
			return {-1, -1};
		}

		static auto get_item_at = mono::get_method("Inventory", "GetItemAt", 2, "assembly_valheim");
		if (!get_item_at)
		{
			return {-1, -1};
		}

		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				slot_kind kind = s_layout[r][c];
				if (kind != slot_kind::none && kind != slot_kind::quick && fits_slot(kind, item_data_obj))
				{
					int x = c, y = r;
					void* args[2] = {&x, &y};
					auto item_at  = mono::invoke_method(get_item_at, equipment_inv, args);
					if (!item_at)
					{
						return {c, r};
					}
				}
			}
		}
		return {-1, -1};
	}

	equipment_manager::~equipment_manager()
	{
		release_handles();
	}

	void equipment_manager::release_handles()
	{
		destroy_native_ui();
		for (int i = 0; i < page_count; ++i)
		{
			if (m_page_handles[i])
			{
				mono::release(m_page_handles[i]);
				m_page_handles[i] = 0;
			}
			m_pages[i] = nullptr;
		}
		m_extra_utilities.clear();
		m_initialized   = false;
		m_cached_player = nullptr;
	}

	MonoObject* equipment_manager::create_page(int index)
	{
		auto inv_class = mono::get_class("Inventory", "assembly_valheim");
		if (!inv_class)
		{
			return nullptr;
		}

		auto inv_obj = mono::object_new(inv_class);
		if (!inv_obj)
		{
			return nullptr;
		}

		auto inv_ctor = mono::class_get_method_from_name(inv_class, ".ctor", 4);
		if (!inv_ctor)
		{
			return nullptr;
		}

		std::string title = (index == equipment_page_idx) ? "$ew_equipment_title" : "$ew_extra_title";
		auto title_mono   = mono::to_mono_string(title);
		void* bkg         = nullptr;
		int w             = grid_width;
		int h             = grid_height;
		void* args[4]     = {title_mono, bkg, &w, &h};
		mono::invoke_method(inv_ctor, inv_obj, args);
		return inv_obj;
	}

	void equipment_manager::ensure_pages()
	{
		for (int i = 0; i < page_count; ++i)
		{
			if (!m_pages[i])
			{
				m_pages[i] = create_page(i);
				if (m_pages[i])
				{
					m_page_handles[i] = mono::retain(m_pages[i]);
				}
			}
		}
	}

	void equipment_manager::init_or_sync()
	{
		if (m_syncing)
		{
			return;
		}

		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			if (m_initialized)
			{
				release_handles();
			}
			return;
		}

		if (m_initialized && m_cached_player == local_player)
		{
			return;
		}

		m_syncing = true;
		release_handles();
		ensure_pages();
		m_cached_player = local_player;
		m_initialized   = true;

		// Read player.m_customData["ew_extra_inventory"]
		auto custom_data = mono::get_field_value<"Player", "m_customData", MonoObject*>(local_player);
		if (custom_data)
		{
			auto dict_class                  = mono::object_get_class(custom_data);
			static MonoMethod* try_get_value = nullptr;
			if (!try_get_value && dict_class)
			{
				try_get_value = mono::class_get_method_from_name(dict_class, "TryGetValue", 2);
				if (!try_get_value)
				{
					void* iter = nullptr;
					while (auto m = mono::class_get_methods(dict_class, &iter))
					{
						if (std::strcmp(mono::method_get_name(m), "TryGetValue") == 0)
						{
							try_get_value = m;
							break;
						}
					}
				}
			}

			if (try_get_value)
			{
				auto key_str        = mono::to_mono_string("ew_extra_inventory");
				MonoString* val_out = nullptr;
				void* args[2]       = {key_str, &val_out};
				auto res            = mono::invoke_method(try_get_value, custom_data, args);
				bool found          = res ? *reinterpret_cast<bool*>(mono::object_unbox(res)) : false;

				if (found && val_out && val_out->length > 0)
				{
					auto zpkg_class = mono::get_class("ZPackage", "assembly_valheim");
					if (zpkg_class)
					{
						auto pkg_obj = mono::object_new(zpkg_class);
						static auto pkg_ctor = mono::get_method_exact("ZPackage", ".ctor", {"string"}, "assembly_valheim");
						if (!pkg_ctor)
						{
							pkg_ctor = mono::get_method_exact("ZPackage", ".ctor", {"System.String"}, "assembly_valheim");
						}

						if (pkg_obj && pkg_ctor)
						{
							void* ctor_args[1] = {val_out};
							mono::invoke_method(pkg_ctor, pkg_obj, ctor_args);

							static auto read_int_m = mono::class_get_method_from_name(zpkg_class, "ReadInt", 0);
							if (read_int_m)
							{
								auto count_res = mono::invoke_method(read_int_m, pkg_obj, nullptr);
								int num_pages  = count_res ? *reinterpret_cast<int*>(mono::object_unbox(count_res)) : 0;

								static auto load_method = mono::get_method("Inventory", "Load", 1, "assembly_valheim");
								if (load_method && num_pages >= 1 && num_pages <= page_count)
								{
									for (int i = 0; i < num_pages; ++i)
									{
										void* load_args[1] = {pkg_obj};
										mono::invoke_method(load_method, m_pages[i], load_args);
									}
									LOG(INFO) << "[EquipmentSlots] Successfully loaded " << num_pages << " pages from player customData!";
								}
							}
						}
					}
				}
			}
		}

		sync_equipment_with_humanoid(local_player);
		m_syncing = false;
	}

	void equipment_manager::sync_equipment_with_humanoid(MonoObject* humanoid)
	{
		if (!humanoid || !m_pages[equipment_page_idx])
		{
			return;
		}

		static auto equip_method = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
		if (!equip_method)
		{
			return;
		}

		m_in_equip = true;
		m_extra_utilities.clear();
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				slot_kind kind = s_layout[r][c];
				auto item      = get_item_at(c, r);
				if (!item)
				{
					continue;
				}

				// Wearable armor/cape/trinket/utility: ensure equipped
				if (kind == slot_kind::helmet || kind == slot_kind::cape || kind == slot_kind::chest || kind == slot_kind::legs || kind == slot_kind::trinket || kind == slot_kind::utility)
				{
					if (kind == slot_kind::utility && g_settings.self.multi_utility_enabled)
					{
						auto current_utility = mono::get_field_value<"Humanoid", "m_utilityItem", MonoObject*>(humanoid);
						if (current_utility && current_utility != item && m_extra_utilities.size() < max_extra_utilities)
						{
							m_extra_utilities.push_back(current_utility);
							mono::set_field_value<"Humanoid", "m_utilityItem">(humanoid, static_cast<MonoObject*>(nullptr));
						}
					}

					// The saved ItemData flag can survive logout while Humanoid's visual
					// equipment references are rebuilt from scratch on the next login.
					// Clear the serialized flag so EquipItem restores those references.
					mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(item, false);
					bool trigger  = false;
					void* args[2] = {item, &trigger};
					mono::invoke_method(equip_method, humanoid, args);
				}
				else
				{
					// Quick slot weapons or ammo that were saved as equipped
					if (is_item_equipped(item))
					{
						mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(item, false);
						bool trigger  = false;
						void* args[2] = {item, &trigger};
						mono::invoke_method(equip_method, humanoid, args);
					}
				}
			}
		}
		m_in_equip = false;

		// Rebuild the player's equipped-item state and VisEquipment models once all
		// restored items have been assigned to the Humanoid fields.
		static auto setup_equipment = mono::get_method("Humanoid", "SetupEquipment", 0, "assembly_valheim");
		if (setup_equipment)
		{
			mono::invoke_method(setup_equipment, humanoid, nullptr);
		}
		on_update_equipment_status_effects(humanoid);
	}

	bool equipment_manager::save_to_player()
	{
		if (!m_initialized || !m_cached_player || m_saving)
		{
			return false;
		}

		m_saving  = true;
		auto fail = [this] {
			m_saving = false;
			return false;
		};

		auto zpkg_class = mono::get_class("ZPackage", "assembly_valheim");
		if (!zpkg_class)
		{
			return fail();
		}

		auto pkg_obj  = mono::object_new(zpkg_class);
		auto pkg_ctor = mono::class_get_method_from_name(zpkg_class, ".ctor", 0);
		if (!pkg_obj || !pkg_ctor)
		{
			return fail();
		}

		MonoObject* exception = nullptr;
		mono::invoke_method(pkg_ctor, pkg_obj, nullptr, &exception);
		if (exception)
		{
			return fail();
		}

		static auto write_int_m = mono::get_method_exact("ZPackage", "Write", {"int"}, "assembly_valheim");
		if (!write_int_m)
		{
			write_int_m = mono::get_method_exact("ZPackage", "Write", {"System.Int32"}, "assembly_valheim");
		}
		if (!write_int_m)
		{
			write_int_m = mono::get_method_overload("ZPackage", "Write", 1, nullptr, "int", "assembly_valheim");
		}

		if (!write_int_m)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve ZPackage.Write(int)!";
			return fail();
		}

		int num_pages     = page_count;
		void* num_args[1] = {&num_pages};
		mono::invoke_method(write_int_m, pkg_obj, num_args, &exception);
		if (exception)
		{
			return fail();
		}

		static auto save_method = mono::get_method("Inventory", "Save", 1, "assembly_valheim");
		if (!save_method)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve Inventory.Save!";
			return fail();
		}

		for (int i = 0; i < page_count; ++i)
		{
			if (!m_pages[i])
			{
				return fail();
			}
			void* save_args[1] = {pkg_obj};
			mono::invoke_method(save_method, m_pages[i], save_args, &exception);
			if (exception)
			{
				return fail();
			}
		}

		static auto get_b64_m = mono::class_get_method_from_name(zpkg_class, "GetBase64", 0);
		if (!get_b64_m)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve ZPackage.GetBase64!";
			return fail();
		}

		auto b64_obj = mono::invoke_method(get_b64_m, pkg_obj, nullptr, &exception);
		if (!b64_obj || exception)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: GetBase64 returned null!";
			return fail();
		}

		auto custom_data = mono::get_field_value<"Player", "m_customData", MonoObject*>(m_cached_player);
		if (!custom_data)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Player.m_customData is null!";
			return fail();
		}

		auto dict_class               = mono::object_get_class(custom_data);
		static MonoMethod* set_item_m = nullptr;
		if (!set_item_m && dict_class)
		{
			set_item_m = mono::class_get_method_from_name(dict_class, "set_Item", 2);
			if (!set_item_m)
			{
				void* iter = nullptr;
				while (auto m = mono::class_get_methods(dict_class, &iter))
				{
					if (std::strcmp(mono::method_get_name(m), "set_Item") == 0)
					{
						set_item_m = m;
						break;
					}
				}
			}
		}

		if (!set_item_m)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve Dictionary.set_Item!";
			return fail();
		}

		auto key1      = mono::to_mono_string("ew_extra_inventory");
		void* args1[2] = {key1, b64_obj};
		mono::invoke_method(set_item_m, custom_data, args1, &exception);
		if (exception)
		{
			return fail();
		}

		auto key2      = mono::to_mono_string("ew_equipment_layout");
		auto val2      = mono::to_mono_string("4");
		void* args2[2] = {key2, val2};
		mono::invoke_method(set_item_m, custom_data, args2, &exception);
		if (exception)
		{
			return fail();
		}

		LOG(INFO) << "[EquipmentSlots] Successfully saved equipment pages to player customData!";
		m_saving = false;
		return true;
	}

	MonoObject* equipment_manager::get_equipment_inventory()
	{
		ensure_pages();
		return m_pages[equipment_page_idx];
	}

	MonoObject* equipment_manager::get_item_at(int x, int y)
	{
		if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
		{
			return nullptr;
		}

		auto eq = get_equipment_inventory();
		if (!eq)
		{
			return nullptr;
		}

		static auto get_item_at_m = mono::get_method("Inventory", "GetItemAt", 2, "assembly_valheim");
		if (!get_item_at_m)
		{
			return nullptr;
		}

		void* args[2] = {&x, &y};
		return mono::invoke_method(get_item_at_m, eq, args);
	}

	bool equipment_manager::move_to_slot(MonoObject* from_inv, MonoObject* item, int to_x, int to_y)
	{
		auto eq = get_equipment_inventory();
		if (!eq || !from_inv || !item)
		{
			return false;
		}

		slot_kind kind = get_slot_kind(to_x, to_y);
		if (!fits_slot(kind, item))
		{
			return false;
		}

		static auto move_method = mono::get_method("Inventory", "MoveItemToThis", 5, "assembly_valheim");
		if (!move_method)
		{
			return false;
		}

		int stack     = get_item_stack(item);
		void* args[5] = {from_inv, item, &stack, &to_x, &to_y};
		auto res      = mono::invoke_method(move_method, eq, args);
		bool success  = res ? *reinterpret_cast<bool*>(mono::object_unbox(res)) : false;

		if (success && m_cached_player)
		{
			if (kind == slot_kind::helmet || kind == slot_kind::cape || kind == slot_kind::chest || kind == slot_kind::legs || kind == slot_kind::trinket || kind == slot_kind::utility)
			{
				m_in_equip          = true;
				static auto equip_m = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
				if (equip_m)
				{
					bool trigger     = true;
					void* eq_args[2] = {item, &trigger};
					mono::invoke_method(equip_m, m_cached_player, eq_args);
				}
				m_in_equip = false;
			}
			save_to_player();
		}
		return success;
	}

	bool equipment_manager::move_from_slot_to_inventory(int from_x, int from_y)
	{
		auto eq = get_equipment_inventory();
		if (!eq || !m_cached_player)
		{
			return false;
		}

		auto item = get_item_at(from_x, from_y);
		if (!item)
		{
			return false;
		}

		if (is_item_equipped(item))
		{
			m_in_equip            = true;
			static auto unequip_m = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
			if (unequip_m)
			{
				bool trigger     = true;
				void* uq_args[2] = {item, &trigger};
				mono::invoke_method(unequip_m, m_cached_player, uq_args);
			}
			m_in_equip = false;
		}

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
		if (!player_inv)
		{
			return false;
		}

		static auto move_m = mono::get_method("Inventory", "MoveItemToThis", 2, "assembly_valheim");
		if (!move_m)
		{
			return false;
		}

		void* args[2] = {eq, item};
		mono::invoke_method(move_m, player_inv, args);
		save_to_player();
		return true;
	}

	void equipment_manager::use_slot(slot_kind kind, int number)
	{
		auto pos = get_slot_pos(kind, number);
		if (pos.x < 0 || pos.y < 0)
		{
			return;
		}

		auto eq   = get_equipment_inventory();
		auto item = get_item_at(pos.x, pos.y);
		if (!eq || !item || !m_cached_player)
		{
			return;
		}

		if (kind == slot_kind::ammo)
		{
			static auto equip_m = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
			if (equip_m)
			{
				bool trigger  = true;
				void* args[2] = {item, &trigger};
				mono::invoke_method(equip_m, m_cached_player, args);
				notification::info("Ammo Slot", std::format("Equipped {} (Slot {})", get_item_display_name(item), number));
			}
		}
		else
		{
			auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
			static auto toggle_equipped_m = mono::get_method("Humanoid", "ToggleEquipped", 1, "assembly_valheim");
			static auto use_m             = mono::get_method("Humanoid", "UseItem", 3, "assembly_valheim");

			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
			int item_type = shared ? mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared) : 0;

			// Food / Consumables (item_type == 2)
			if (item_type == 2)
			{
				if (use_m && player_inv)
				{
					bool from_gui = true;
					void* args[3] = {player_inv, item, &from_gui};
					mono::invoke_method(use_m, m_cached_player, args);
				}
			}
			else
			{
				// Weapon, Shield, Tool, Armor, Utility
				if (toggle_equipped_m)
				{
					void* args[1] = {item};
					mono::invoke_method(toggle_equipped_m, m_cached_player, args);
				}
				else if (use_m && player_inv)
				{
					bool from_gui = true;
					void* args[3] = {player_inv, item, &from_gui};
					mono::invoke_method(use_m, m_cached_player, args);
				}
			}

			if (kind == slot_kind::quick)
			{
				bool now_equipped = is_item_equipped(item);
				notification::info("Quick Slot", std::format("{} {}", now_equipped ? "Equipped" : "Unequipped", get_item_display_name(item)));
			}
		}
		save_to_player();
	}

	void equipment_manager::use_hotbar_index(int index, bool alt_held, bool shift_held)
	{
		if (alt_held && index >= 1 && index <= 6)
		{
			use_slot(slot_kind::quick, index);
		}
		else if (shift_held && index >= 1 && index <= 3)
		{
			use_slot(slot_kind::food, index);
		}
	}

	void equipment_manager::auto_fill_from_inventory()
	{
		auto player = unity::get_local_player();
		auto eq     = get_equipment_inventory();
		if (!player || !eq)
		{
			return;
		}

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
		if (!player_inv)
		{
			return;
		}

		static auto get_all_m = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
		if (!get_all_m)
		{
			return;
		}

		auto all_items = mono::invoke_method(get_all_m, player_inv, nullptr);
		if (!all_items)
		{
			return;
		}

		auto items_vec  = unity::list_to_vector(all_items);
		int moved_count = 0;
		std::unordered_set<MonoObject*> moved_items;

		// Pass 1: Prioritize currently equipped items so worn gear moves into dedicated slots first
		for (auto* itm : items_vec)
		{
			if (!itm || !is_item_equipped(itm))
			{
				continue;
			}

			auto home = find_home_slot(eq, itm);
			if (home.x >= 0 && home.y >= 0)
			{
				auto existing = get_item_at(home.x, home.y);
				if (!existing)
				{
					if (move_to_slot(player_inv, itm, home.x, home.y))
					{
						moved_count++;
						moved_items.insert(itm);
					}
				}
			}
		}

		// Pass 2: Fill remaining empty slots with unequipped items (food, ammo, keys, extra gear)
		for (auto* itm : items_vec)
		{
			if (!itm || moved_items.contains(itm) || is_item_equipped(itm))
			{
				continue;
			}

			auto home = find_home_slot(eq, itm);
			if (home.x >= 0 && home.y >= 0)
			{
				auto existing = get_item_at(home.x, home.y);
				if (!existing)
				{
					if (move_to_slot(player_inv, itm, home.x, home.y))
					{
						moved_count++;
						moved_items.insert(itm);
					}
				}
			}
		}

		if (moved_count > 0)
		{
			notification::success("Equipment Slots", std::format("Auto-filled {} items into dedicated slots!", moved_count));
		}
		else
		{
			notification::info("Equipment Slots", "No eligible items to auto-fill.");
		}
	}

	void equipment_manager::deposit_all_to_inventory()
	{
		auto player = unity::get_local_player();
		auto eq     = get_equipment_inventory();
		if (!player || !eq)
		{
			return;
		}

		int deposited = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (get_item_at(c, r))
				{
					if (move_from_slot_to_inventory(c, r))
					{
						deposited++;
					}
				}
			}
		}
		if (deposited > 0)
		{
			notification::info("Equipment Slots", std::format("Deposited {} items back to inventory.", deposited));
		}
	}

	void equipment_manager::unequip_all()
	{
		if (!m_cached_player)
		{
			return;
		}

		static auto unequip_all_m = mono::get_method("Humanoid", "UnequipAllItems", 0, "assembly_valheim");
		if (unequip_all_m)
		{
			mono::invoke_method(unequip_all_m, m_cached_player, nullptr);
		}

		for (auto* util : m_extra_utilities)
		{
			if (util)
			{
				mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(util, false);
			}
		}
		m_extra_utilities.clear();
	}

	void equipment_manager::tick(MonoObject* player)
	{
		if (!g_settings.self.equipment_slots_enabled)
		{
			if (m_native_grid)
			{
				destroy_native_ui();
			}
			return;
		}

		init_or_sync();
		if (!m_initialized || player != m_cached_player)
		{
			return;
		}

		// Update Native Unity UI
		update_native_ui(player);

		// Hotkey listener for Ammo switching (Alt + R, Alt + T, Alt + Z)
		if (g_settings.self.equipment_slots_hotkeys)
		{
			bool alt_down = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			if (alt_down)
			{
				bool r_down = (GetAsyncKeyState('R') & 0x8000) != 0;
				if (r_down && !m_r_key_down)
				{
					use_slot(slot_kind::ammo, 1);
				}
				m_r_key_down = r_down;

				bool t_down = (GetAsyncKeyState('T') & 0x8000) != 0;
				if (t_down && !m_t_key_down)
				{
					use_slot(slot_kind::ammo, 2);
				}
				m_t_key_down = t_down;

				bool z_down = ((GetAsyncKeyState('Z') & 0x8000) != 0) || ((GetAsyncKeyState('Y') & 0x8000) != 0);
				if (z_down && !m_z_key_down)
				{
					use_slot(slot_kind::ammo, 3);
				}
				m_z_key_down = z_down;
			}
			else
			{
				m_r_key_down = false;
				m_t_key_down = false;
				m_z_key_down = false;
			}
		}
	}

	void equipment_manager::send_to_slot(MonoObject* item)
	{
		if (!item || !m_cached_player)
		{
			return;
		}

		auto eq = get_equipment_inventory();
		if (!eq)
		{
			return;
		}

		auto home = find_home_slot(eq, item);
		if (home.x < 0 || home.y < 0)
		{
			return;
		}

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
		if (!player_inv)
		{
			return;
		}

		auto existing = get_item_at(home.x, home.y);
		if (existing == item)
		{
			return;
		}

		move_to_slot(player_inv, item, home.x, home.y);
	}

	bool equipment_manager::handle_inventory_equip(MonoObject* humanoid, MonoObject* item)
	{
		if (!g_settings.self.equipment_slots_enabled || m_in_equip || !m_initialized)
		{
			return false;
		}

		if (!humanoid || humanoid != m_cached_player || !item)
		{
			return false;
		}

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(humanoid);
		if (!player_inv)
		{
			return false;
		}

		auto eq = get_equipment_inventory();
		if (!eq)
		{
			return false;
		}

		// Verify that this item is in the main player inventory
		if (!detour_base::get_original<hooks::inventory_contains_item>()(player_inv, item))
		{
			return false;
		}

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
		if (!shared)
		{
			return false;
		}

		int item_type = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared);

		iVector2 target_slot = {-1, -1};
		if (item_type == 6)
		{
			target_slot = {0, 0}; // Helmet
		}
		else if (item_type == 17)
		{
			target_slot = {0, 1}; // Cape
		}
		else if (item_type == 7)
		{
			target_slot = {0, 2}; // Chest
		}
		else if (item_type == 11)
		{
			target_slot = {0, 3}; // Legs
		}
		else if (item_type == 24)
		{
			target_slot = {1, 2}; // Trinket
		}
		else if (item_type == 18) // Utility
		{
			if (g_settings.self.multi_utility_enabled)
			{
				const iVector2 util_slots[] = {{1, 0}, {2, 0}, {1, 1}, {2, 1}};
				for (auto s : util_slots)
				{
					if (!get_item_at(s.x, s.y))
					{
						target_slot = s;
						break;
					}
				}
				if (target_slot.x == -1)
				{
					target_slot = util_slots[0];
				}
			}
			else
			{
				target_slot = {1, 0};
			}
		}
		else
		{
			return false; // Not a wearable item managed by dedicated slots
		}

		m_in_equip = true;

		static auto move_5_m  = mono::get_method("Inventory", "MoveItemToThis", 5, "assembly_valheim");
		static auto move_2_m  = mono::get_method("Inventory", "MoveItemToThis", 2, "assembly_valheim");
		static auto unequip_m = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
		static auto equip_m   = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");

		auto existing = get_item_at(target_slot.x, target_slot.y);
		if (existing)
		{
			// 1. Unequip existing item in that slot
			if (is_item_equipped(existing) && unequip_m)
			{
				bool trigger     = true;
				void* uq_args[2] = {existing, &trigger};
				mono::invoke_method(unequip_m, humanoid, uq_args);
			}

			// 2. Move existing to temp slot (3, 0) in eq
			if (move_5_m)
			{
				int temp_x = 3, temp_y = 0;
				int stack      = get_item_stack(existing);
				void* args1[5] = {eq, existing, &stack, &temp_x, &temp_y};
				mono::invoke_method(move_5_m, eq, args1);
			}

			// 3. Move new item from player_inv to target_slot in eq
			if (move_5_m)
			{
				int stack      = get_item_stack(item);
				void* args2[5] = {player_inv, item, &stack, &target_slot.x, &target_slot.y};
				mono::invoke_method(move_5_m, eq, args2);
			}

			// 4. Move existing from eq into player_inv
			if (move_2_m)
			{
				void* args3[2] = {eq, existing};
				mono::invoke_method(move_2_m, player_inv, args3);
			}
		}
		else
		{
			// Target slot is empty, move new item directly to target_slot
			if (move_5_m)
			{
				int stack     = get_item_stack(item);
				void* args[5] = {player_inv, item, &stack, &target_slot.x, &target_slot.y};
				mono::invoke_method(move_5_m, eq, args);
			}
		}

		// 5. Equip new item
		if (equip_m)
		{
			bool trigger     = true;
			void* eq_args[2] = {item, &trigger};
			mono::invoke_method(equip_m, humanoid, eq_args);
		}

		save_to_player();
		m_in_equip = false;
		return true;
	}

	bool equipment_manager::on_equip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects)
	{
		if (!g_settings.self.multi_utility_enabled || !humanoid || humanoid != m_cached_player || !item)
		{
			return false;
		}

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
		if (!shared)
		{
			return false;
		}

		int item_type = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared);
		if (item_type != 18) // ItemType.Utility
		{
			return false;
		}

		auto current_util = mono::get_field_value<"Humanoid", "m_utilityItem", MonoObject*>(humanoid);
		if (current_util && current_util != item)
		{
			// Check if already in extras
			if (std::find(m_extra_utilities.begin(), m_extra_utilities.end(), current_util) == m_extra_utilities.end())
			{
				if (m_extra_utilities.size() < max_extra_utilities)
				{
					m_extra_utilities.push_back(current_util);
					// Temporarily clear m_utilityItem so vanilla EquipItem doesn't unequip it!
					mono::set_field_value<"Humanoid", "m_utilityItem">(humanoid, (MonoObject*)nullptr);
					return true;
				}
			}
		}
		return false;
	}

	bool equipment_manager::on_unequip_item(MonoObject* humanoid, MonoObject* item, bool trigger_equip_effects)
	{
		if (!humanoid || humanoid != m_cached_player || !item)
		{
			return false;
		}

		auto it = std::find(m_extra_utilities.begin(), m_extra_utilities.end(), item);
		if (it != m_extra_utilities.end())
		{
			m_extra_utilities.erase(it);
			mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(item, false);

			// Remove status effect
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
			if (shared)
			{
				auto se = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_equipStatusEffect", MonoObject*>(shared);
				auto seman = mono::get_field_value<"Character", "m_seman", MonoObject*>(humanoid);
				if (se && seman)
				{
					static auto remove_se_m = mono::get_method_exact("SEMan", "RemoveStatusEffect", {"StatusEffect", "System.Boolean"}, "assembly_valheim");
					if (remove_se_m)
					{
						bool quiet    = false;
						void* args[2] = {se, &quiet};
						mono::invoke_method(remove_se_m, seman, args);
					}
				}
			}
			return true;
		}

		auto current_util = mono::get_field_value<"Humanoid", "m_utilityItem", MonoObject*>(humanoid);
		if (current_util == item && !m_extra_utilities.empty())
		{
			auto next_util = m_extra_utilities.back();
			m_extra_utilities.pop_back();
			mono::set_field_value<"Humanoid", "m_utilityItem">(humanoid, next_util);
		}
		return false;
	}

	void equipment_manager::on_update_equipment_status_effects(MonoObject* humanoid)
	{
		if (!humanoid || humanoid != m_cached_player || m_extra_utilities.empty())
		{
			return;
		}

		auto seman         = mono::get_field_value<"Character", "m_seman", MonoObject*>(humanoid);
		auto equip_effects = mono::get_field_value<"Humanoid", "m_equipmentStatusEffects", MonoObject*>(humanoid);
		if (!seman)
		{
			return;
		}

		static auto name_hash_m = mono::get_method("StatusEffect", "NameHash", 0, "assembly_valheim");
		static auto have_se_m = mono::get_method_exact("SEMan", "HaveStatusEffect", {"System.Int32"}, "assembly_valheim");
		static auto add_se_m = []() {
			auto m = mono::get_method_exact("SEMan", "AddStatusEffect", {"StatusEffect", "System.Boolean", "System.Int32", "System.Single", "System.Int16"}, "assembly_valheim");
			if (!m)
			{
				m = mono::get_method_exact("SEMan", "AddStatusEffect", {"StatusEffect", "System.Boolean", "System.Int32", "System.Single"}, "assembly_valheim");
			}
			return m;
		}();

		for (auto* util : m_extra_utilities)
		{
			if (!util)
			{
				continue;
			}
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(util);
			if (!shared)
			{
				continue;
			}

			auto eq_se = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_equipStatusEffect", MonoObject*>(shared);
			if (eq_se && name_hash_m && have_se_m && add_se_m)
			{
				auto hash_result = mono::invoke_method(name_hash_m, eq_se, nullptr);
				if (!hash_result || !mono::object_unbox(hash_result))
				{
					continue;
				}
				int effect_hash    = *static_cast<int*>(mono::object_unbox(hash_result));
				void* have_args[1] = {&effect_hash};
				auto have_res      = mono::invoke_method(have_se_m, seman, have_args);
				bool has           = have_res ? *reinterpret_cast<bool*>(mono::object_unbox(have_res)) : false;

				if (!has)
				{
					bool reset_time = false;
					int item_lvl    = 0;
					float skill_lvl = 0.f;
					short variant   = -1;
					if (mono::get_param_count(add_se_m) == 5)
					{
						void* add_args[5] = {eq_se, &reset_time, &item_lvl, &skill_lvl, &variant};
						mono::invoke_method(add_se_m, seman, add_args);
					}
					else
					{
						void* add_args[4] = {eq_se, &reset_time, &item_lvl, &skill_lvl};
						mono::invoke_method(add_se_m, seman, add_args);
					}
				}

				if (equip_effects)
				{
					auto hs_class        = mono::object_get_class(equip_effects);
					static auto add_hs_m = hs_class ? mono::class_get_method_from_name(hs_class, "Add", 1) : nullptr;
					if (add_hs_m)
					{
						void* hs_args[1] = {eq_se};
						mono::invoke_method(add_hs_m, equip_effects, hs_args);
					}
				}
			}
		}
	}

	void equipment_manager::on_unequip_all(MonoObject* humanoid)
	{
		if (humanoid != m_cached_player)
		{
			return;
		}
		for (auto* util : m_extra_utilities)
		{
			if (util)
			{
				mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(util, false);
			}
		}
		m_extra_utilities.clear();
	}

	void equipment_manager::on_player_save(MonoObject* player, MonoObject* pkg)
	{
		if (player == m_cached_player)
		{
			save_to_player();
		}
	}

	void equipment_manager::destroy_native_ui()
	{
		if (m_native_grid_handle)
		{
			auto grid = mono::retained_object(m_native_grid_handle);
			if (grid)
			{
				static auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
				static auto destroy_m = mono::get_method("Object", "Destroy", 1, "UnityEngine.CoreModule", "UnityEngine");
				if (get_go && destroy_m)
				{
					auto go = mono::invoke_method(get_go, grid, nullptr);
					if (go)
					{
						void* dargs[1] = {go};
						mono::invoke_method(destroy_m, nullptr, dargs);
					}
				}
			}
			mono::release(m_native_grid_handle);
			m_native_grid_handle = 0;
		}
		m_native_grid          = nullptr;
		m_cached_inventory_gui = nullptr;
	}

	void equipment_manager::update_native_grid_position(MonoObject* gui)
	{
		if (!m_native_grid)
		{
			return;
		}

		static auto get_transform = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto set_local_pos = mono::get_method("Transform", "set_localPosition", 1, "UnityEngine.CoreModule", "UnityEngine");
		if (!get_transform || !set_local_pos)
		{
			return;
		}

		auto clone_t = mono::invoke_method(get_transform, m_native_grid, nullptr);
		if (!clone_t)
		{
			return;
		}

		Vector3 pos{g_settings.self.equipment_slots_native_x, g_settings.self.equipment_slots_native_y, 0.f};
		void* args[1] = {&pos};
		mono::invoke_method(set_local_pos, clone_t, args);
	}

	void equipment_manager::create_native_grid(MonoObject* gui)
	{
		destroy_native_ui();
		m_cached_inventory_gui = gui;

		auto player_rect = mono::get_field_value<"InventoryGui", "m_player", MonoObject*>(gui);
		auto player_grid = mono::get_field_value<"InventoryGui", "m_playerGrid", MonoObject*>(gui);
		if (!player_rect || !player_grid)
		{
			return;
		}

		static auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_transform = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
		if (!get_go || !get_transform)
		{
			return;
		}

		auto player_grid_go = mono::invoke_method(get_go, player_grid, nullptr);
		auto player_rect_t  = mono::invoke_method(get_transform, player_rect, nullptr);
		if (!player_grid_go || !player_rect_t)
		{
			return;
		}

		auto instantiate_m = mono::get_method_exact("Object", "Instantiate", {"UnityEngine.Object", "UnityEngine.Transform", "System.Boolean"}, "UnityEngine.CoreModule", "UnityEngine");
		if (!instantiate_m)
		{
			instantiate_m = mono::get_method_overload("Object", "Instantiate", 2, nullptr, "Transform", "UnityEngine.CoreModule", "UnityEngine");
		}
		if (!instantiate_m)
		{
			return;
		}

		bool world_pos     = false;
		void* inst_args[3] = {player_grid_go, player_rect_t, &world_pos};
		auto clone_go      = mono::invoke_method(instantiate_m, nullptr, inst_args);
		if (!clone_go)
		{
			return;
		}

		static auto get_comp = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		auto grid_class = mono::get_class("InventoryGrid", "assembly_valheim");
		if (!get_comp || !grid_class)
		{
			return;
		}

		auto grid_type  = mono::reflection_type(grid_class);
		void* targs[1]  = {grid_type};
		auto clone_grid = mono::invoke_method(get_comp, clone_go, targs);
		if (!clone_grid)
		{
			return;
		}

		m_native_grid        = clone_grid;
		m_native_grid_handle = mono::retain(m_native_grid);

		auto grid_root = mono::get_field_value<"InventoryGrid", "m_gridRoot", MonoObject*>(m_native_grid);
		static auto get_game_object = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		static auto add_component = mono::get_method_overload("GameObject", "AddComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		static auto image_class = mono::get_class("Image", "UnityEngine.UI", "UnityEngine.UI");
		static auto set_color   = mono::get_method("Graphic", "set_color", 1, "UnityEngine.UI", "UnityEngine.UI");
		static auto set_raycast_target = mono::get_method("Graphic", "set_raycastTarget", 1, "UnityEngine.UI", "UnityEngine.UI");
		static auto get_sprite     = mono::get_method("Image", "get_sprite", 0, "UnityEngine.UI", "UnityEngine.UI");
		static auto set_sprite     = mono::get_method("Image", "set_sprite", 1, "UnityEngine.UI", "UnityEngine.UI");
		static auto get_image_type = mono::get_method("Image", "get_type", 0, "UnityEngine.UI", "UnityEngine.UI");
		static auto set_image_type = mono::get_method("Image", "set_type", 1, "UnityEngine.UI", "UnityEngine.UI");
		if (grid_root && get_game_object && get_component && add_component && image_class && set_color && set_raycast_target)
		{
			auto grid_root_go  = mono::invoke_method(get_game_object, grid_root, nullptr);
			auto image_type    = mono::reflection_type(image_class);
			void* type_args[1] = {image_type};
			auto background    = grid_root_go ? mono::invoke_method(get_component, grid_root_go, type_args) : nullptr;
			if (!background && grid_root_go)
			{
				background = mono::invoke_method(add_component, grid_root_go, type_args);
			}

			if (background)
			{
				unity_color color{0.13f, 0.14f, 0.13f, 0.94f};
				auto player_go         = mono::invoke_method(get_game_object, player_rect, nullptr);
				auto source_background = player_go ? mono::invoke_method(get_component, player_go, type_args) : nullptr;
				if (source_background && get_sprite && set_sprite)
				{
					auto sprite = mono::invoke_method(get_sprite, source_background, nullptr);
					if (sprite)
					{
						static auto get_color = mono::get_method("Graphic", "get_color", 0, "UnityEngine.UI", "UnityEngine.UI");
						if (get_color)
						{
							auto boxed_color = mono::invoke_method(get_color, source_background, nullptr);
							if (boxed_color)
							{
								color = *static_cast<unity_color*>(mono::object_unbox(boxed_color));
							}
						}

						void* sprite_args[1] = {sprite};
						mono::invoke_method(set_sprite, background, sprite_args);
						if (get_image_type && set_image_type)
						{
							auto boxed_type = mono::invoke_method(get_image_type, source_background, nullptr);
							if (boxed_type)
							{
								int source_type           = *static_cast<int*>(mono::object_unbox(boxed_type));
								void* source_type_args[1] = {&source_type};
								mono::invoke_method(set_image_type, background, source_type_args);
							}
						}
					}
				}

				void* color_args[1] = {&color};
				mono::invoke_method(set_color, background, color_args);
				bool raycast_target   = false;
				void* raycast_args[1] = {&raycast_target};
				mono::invoke_method(set_raycast_target, background, raycast_args);
			}
		}

		static auto set_name = mono::get_method("Object", "set_name", 1, "UnityEngine.CoreModule", "UnityEngine");
		if (set_name)
		{
			auto name_str  = mono::to_mono_string("EquipmentGrid");
			void* sargs[1] = {name_str};
			mono::invoke_method(set_name, clone_go, sargs);
		}

		// Copy event delegates from player_grid so clicks/drags route to InventoryGui
		auto f_selected = mono::get_field(grid_class, "m_onSelected");
		auto f_right    = mono::get_field(grid_class, "m_onRightClick");
		if (f_selected)
		{
			MonoObject* sel_del = nullptr;
			mono::get_field_value(player_grid, f_selected, &sel_del);
			if (sel_del)
			{
				mono::set_field_value(clone_grid, f_selected, &sel_del);
			}
		}
		if (f_right)
		{
			MonoObject* right_del = nullptr;
			mono::get_field_value(player_grid, f_right, &right_del);
			if (right_del)
			{
				mono::set_field_value(clone_grid, f_right, &right_del);
			}
		}

		update_native_grid_position(gui);
	}

	void equipment_manager::decorate_native_slots()
	{
		if (!m_native_grid)
		{
			return;
		}

		auto grid_class = mono::get_class("InventoryGrid", "assembly_valheim");
		if (!grid_class)
		{
			return;
		}

		static auto f_elements = mono::get_field(grid_class, "m_elements");
		if (!f_elements)
		{
			return;
		}

		MonoObject* list_obj = nullptr;
		mono::get_field_value(m_native_grid, f_elements, &list_obj);
		if (!list_obj)
		{
			return;
		}

		auto elements = unity::list_to_vector(list_obj);
		if (elements.empty())
		{
			return;
		}

		auto elem_class      = mono::get_class("InventoryElement", "assembly_valheim");
		bool is_mono_element = (elem_class != nullptr);
		if (!elem_class)
		{
			elem_class = mono::get_class("InventoryGrid/Element", "assembly_valheim");
		}
		if (!elem_class)
		{
			return;
		}

		static auto f_pos = mono::get_field(elem_class, "m_pos");
		static auto get_pos_m = is_mono_element ? mono::get_method("InventoryElement", "get_Position", 0, "assembly_valheim") : nullptr;
		static auto f_go = is_mono_element ? nullptr : mono::get_field(elem_class, "m_go");
		static auto get_go_m = is_mono_element ? mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine") : nullptr;
		static auto f_used = mono::get_field(elem_class, "m_used");
		static auto set_active = mono::get_method("GameObject", "SetActive", 1, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_transform = mono::get_method("GameObject", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto find_child = mono::get_method("Transform", "Find", 1, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_comp = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");

		auto tmp_class = mono::get_class("TMP_Text", "Unity.TextMeshPro", "TMPro");
		if (!tmp_class)
		{
			tmp_class = mono::get_class("TMP_Text", "assembly_valheim", "TMPro");
		}

		static auto set_text_m = tmp_class ? mono::get_method("TMP_Text", "set_text", 1, "Unity.TextMeshPro", "TMPro") : nullptr;
		static auto set_enabled_m = mono::get_method("Behaviour", "set_enabled", 1, "UnityEngine.CoreModule", "UnityEngine");

		for (auto* el : elements)
		{
			if (!el)
			{
				continue;
			}

			iVector2 pos{-1, -1};
			if (get_pos_m)
			{
				auto pos_obj = mono::invoke_method(get_pos_m, el, nullptr);
				if (pos_obj)
				{
					pos = *reinterpret_cast<iVector2*>(mono::object_unbox(pos_obj));
				}
			}
			else if (f_pos)
			{
				mono::get_field_value(el, f_pos, &pos);
			}

			bool used = false;
			if (f_used)
			{
				mono::get_field_value(el, f_used, &used);
			}

			MonoObject* go = nullptr;
			if (get_go_m)
			{
				go = mono::invoke_method(get_go_m, el, nullptr);
			}
			else if (f_go)
			{
				mono::get_field_value(el, f_go, &go);
			}

			if (!go)
			{
				continue;
			}

			slot_kind kind = get_slot_kind(pos.x, pos.y);

			if (kind == slot_kind::none)
			{
				if (set_active)
				{
					bool act       = false;
					void* aargs[1] = {&act};
					mono::invoke_method(set_active, go, aargs);
				}
				continue;
			}

			if (set_active)
			{
				bool act       = true;
				void* aargs[1] = {&act};
				mono::invoke_method(set_active, go, aargs);
			}

			if (get_transform && find_child && get_comp && tmp_class)
			{
				auto tr = mono::invoke_method(get_transform, go, nullptr);
				if (tr)
				{
					auto ms_bind   = mono::to_mono_string("binding");
					void* bargs[1] = {ms_bind};
					auto b_t       = mono::invoke_method(find_child, tr, bargs);
					if (b_t)
					{
						auto t_type    = mono::reflection_type(tmp_class);
						void* cargs[1] = {t_type};
						auto tmp_comp  = mono::invoke_method(get_comp, b_t, cargs);
						if (tmp_comp)
						{
							std::string tag;
							int num = get_slot_number(kind, pos.x, pos.y);
							if (!used)
							{
								switch (kind)
								{
								case slot_kind::helmet: tag = "<color=#E0B858>HELM</color>"; break;
								case slot_kind::cape: tag = "<color=#E0B858>CAPE</color>"; break;
								case slot_kind::chest: tag = "<color=#E0B858>CHEST</color>"; break;
								case slot_kind::legs: tag = "<color=#E0B858>LEGS</color>"; break;
								case slot_kind::trinket: tag = "<color=#E0B858>TRINK</color>"; break;
								case slot_kind::utility:
									tag = (num == 1) ? "<color=#58B8C8>UTIL 1</color>" :
									    (num == 2)   ? "<color=#58B8C8>UTIL 2</color>" :
									    (num == 3)   ? "<color=#58B8C8>UTIL 3</color>" :
									                   "<color=#58B8C8>UTIL 4</color>";
									break;
								case slot_kind::food:
									tag = (num == 1) ? "<color=#72BA56>FOOD 1</color>" :
									    (num == 2)   ? "<color=#72BA56>FOOD 2</color>" :
									                   "<color=#72BA56>FOOD 3</color>";
									break;
								case slot_kind::ammo:
									tag = (num == 1) ? "<color=#D96E34>AMMO 1</color>" :
									    (num == 2)   ? "<color=#D96E34>AMMO 2</color>" :
									                   "<color=#D96E34>AMMO 3</color>";
									break;
								case slot_kind::key:
									tag = (num == 1) ? "<color=#D9AE36>KEY 1</color>" : "<color=#D9AE36>KEY 2</color>";
									break;
								case slot_kind::quick:
									tag = (num == 1) ? "<color=#9D78D6>Q1</color>" :
									    (num == 2)   ? "<color=#9D78D6>Q2</color>" :
									    (num == 3)   ? "<color=#9D78D6>Q3</color>" :
									    (num == 4)   ? "<color=#9D78D6>Q4</color>" :
									    (num == 5)   ? "<color=#9D78D6>Q5</color>" :
									                   "<color=#9D78D6>Q6</color>";
									break;
								default: break;
								}
							}
							else if (g_settings.self.equipment_slots_hotkeys)
							{
								switch (kind)
								{
								case slot_kind::quick:
									tag = (num == 1) ? "<color=#9D78D6>Alt+1</color>" :
									    (num == 2)   ? "<color=#9D78D6>Alt+2</color>" :
									    (num == 3)   ? "<color=#9D78D6>Alt+3</color>" :
									    (num == 4)   ? "<color=#9D78D6>Alt+4</color>" :
									    (num == 5)   ? "<color=#9D78D6>Alt+5</color>" :
									                   "<color=#9D78D6>Alt+6</color>";
									break;
								case slot_kind::food:
									tag = (num == 1) ? "<color=#72BA56>Shift+1</color>" :
									    (num == 2)   ? "<color=#72BA56>Shift+2</color>" :
									                   "<color=#72BA56>Shift+3</color>";
									break;
								case slot_kind::ammo:
									tag = (num == 1) ? "<color=#D96E34>Alt+R</color>" :
									    (num == 2)   ? "<color=#D96E34>Alt+T</color>" :
									                   "<color=#D96E34>Alt+Z</color>";
									break;
								default: break;
								}
							}

							if (set_text_m && !tag.empty())
							{
								auto ms         = mono::to_mono_string(tag);
								void* t_args[1] = {ms};
								mono::invoke_method(set_text_m, tmp_comp, t_args);

								if (set_enabled_m)
								{
									bool en         = true;
									void* e_args[1] = {&en};
									mono::invoke_method(set_enabled_m, tmp_comp, e_args);
								}
							}
							else if (set_enabled_m && used)
							{
								bool en         = false;
								void* e_args[1] = {&en};
								mono::invoke_method(set_enabled_m, tmp_comp, e_args);
							}
						}
					}
				}
			}
		}
	}

	void equipment_manager::update_native_ui(MonoObject* player)
	{
		if (!g_settings.self.equipment_slots_enabled || !g_settings.self.equipment_slots_native_ui)
		{
			if (m_native_grid)
			{
				static auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
				static auto set_active = mono::get_method("GameObject", "SetActive", 1, "UnityEngine.CoreModule", "UnityEngine");
				if (get_go && set_active)
				{
					auto go = mono::invoke_method(get_go, m_native_grid, nullptr);
					if (go)
					{
						bool act       = false;
						void* aargs[1] = {&act};
						mono::invoke_method(set_active, go, aargs);
					}
				}
			}
			return;
		}

		auto gui = unity::get_inventory_gui();
		if (!gui)
		{
			if (m_native_grid)
			{
				destroy_native_ui();
			}
			return;
		}

		bool is_vis = unity::is_inventory_gui_visible();
		if (!is_vis)
		{
			if (m_native_grid)
			{
				static auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
				static auto set_active = mono::get_method("GameObject", "SetActive", 1, "UnityEngine.CoreModule", "UnityEngine");
				if (get_go && set_active)
				{
					auto go = mono::invoke_method(get_go, m_native_grid, nullptr);
					if (go)
					{
						bool act       = false;
						void* aargs[1] = {&act};
						mono::invoke_method(set_active, go, aargs);
					}
				}
			}
			return;
		}

		if (!m_native_grid || m_cached_inventory_gui != gui)
		{
			create_native_grid(gui);
		}

		if (!m_native_grid)
		{
			return;
		}

		static auto get_go = mono::get_method("Component", "get_gameObject", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto set_active = mono::get_method("GameObject", "SetActive", 1, "UnityEngine.CoreModule", "UnityEngine");
		if (get_go && set_active)
		{
			auto go = mono::invoke_method(get_go, m_native_grid, nullptr);
			if (go)
			{
				bool act       = true;
				void* aargs[1] = {&act};
				mono::invoke_method(set_active, go, aargs);
			}
		}

		update_native_grid_position(gui);

		auto eq_inv = get_equipment_inventory();
		if (!eq_inv)
		{
			return;
		}

		auto drag_item = mono::get_field_value<"InventoryGui", "m_dragItem", MonoObject*>(gui);

		static auto update_inv_m = mono::get_method("InventoryGrid", "UpdateInventory", 3, "assembly_valheim");
		if (update_inv_m)
		{
			void* uargs[3] = {eq_inv, player, drag_item};
			mono::invoke_method(update_inv_m, m_native_grid, uargs);
		}

		decorate_native_slots();
	}

	void equipment_manager::draw_ui()
	{
		if (!g_settings.self.equipment_slots_enabled || !g_settings.self.equipment_slots_window || !unity::is_inventory_gui_visible())
		{
			return;
		}

		auto& mgr = get();
		mgr.init_or_sync();

		auto player = unity::get_local_player();
		if (!player)
		{
			return;
		}

		if (!canvas::begin_panel("Equipment & Quick Slots", &g_settings.self.equipment_slots_window, {.size = {760.f, 600.f}, .collapsible = false}))
		{
			canvas::end_panel();
			return;
		}

		const auto theme = canvas::current_theme();
		const ImVec4 accent{theme.accent.x, theme.accent.y, theme.accent.z, 1.f};
		ImGui::TextColored(accent, "Equipment & quick slots");
		ImGui::SameLine();
		ImGui::TextDisabled("Gear, food, ammunition, and quick access");
		ImGui::Spacing();

		if (ImGui::Button("Auto-Fill", ImVec2(110.f, 0.f)))
		{
			mgr.auto_fill_from_inventory();
		}
		ImGui::SameLine();
		if (ImGui::Button("Return All", ImVec2(110.f, 0.f)))
		{
			mgr.deposit_all_to_inventory();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Loadout", ImVec2(125.f, 0.f)))
		{
			if (mgr.save_to_player())
			{
				notification::success("Equipment Slots", "Saved equipment slots to character!");
			}
			else
			{
				notification::warning("Equipment Slots", "Could not save the loadout. Join a world and try again.");
			}
		}
		ImGui::SameLine();
		ImGui::Checkbox("Hotkeys", &g_settings.self.equipment_slots_hotkeys);
		ImGui::Separator();

		auto draw_slot = [&](int c, int r) {
			const auto kind = s_layout[r][c];
			if (kind == slot_kind::none)
			{
				return;
			}
			const int num = get_slot_number(kind, c, r);
			auto item     = mgr.get_item_at(c, r);
			std::string title;
			std::string hotkey;
			switch (kind)
			{
			case slot_kind::helmet: title = "HELM"; break;
			case slot_kind::cape: title = "CAPE"; break;
			case slot_kind::chest: title = "CHEST"; break;
			case slot_kind::legs: title = "LEGS"; break;
			case slot_kind::trinket: title = "TRINKET"; break;
			case slot_kind::utility: title = std::format("UTIL {}", num); break;
			case slot_kind::food:
				title  = std::format("FOOD {}", num);
				hotkey = std::format("Shift+{}", num);
				break;
			case slot_kind::ammo:
				title  = std::format("AMMO {}", num);
				hotkey = std::format("Alt+{}", num == 1 ? "R" : num == 2 ? "T" : "Z");
				break;
			case slot_kind::key: title = std::format("KEY {}", num); break;
			case slot_kind::quick:
				title  = std::format("QUICK {}", num);
				hotkey = std::format("Alt+{}", num);
				break;
			default: break;
			}

			ImGui::PushID(r * grid_width + c);
			ImGui::BeginGroup();
			ImGui::TextUnformatted(title.c_str());
			// The border marks the slot's state in the menu theme: equipped, holding an item, or empty.
			const bool equipped = item && is_item_equipped(item);
			const ImVec4 border = equipped ? accent :
			    item ? ImVec4(theme.muted.x, theme.muted.y, theme.muted.z, 0.85f) :
			           ImVec4(theme.muted.x, theme.muted.y, theme.muted.z, 0.35f);
			ImGui::PushStyleColor(ImGuiCol_Border, border);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
			bool clicked = false;
			if (item)
			{
				const auto prefab  = get_item_prefab_name(item);
				const auto texture = item_icons::get(prefab);
				if (texture)
				{
					clicked = ImGui::ImageButton("##slot_icon", texture, ImVec2(56.f, 56.f));
				}
				else
				{
					std::string short_name = get_item_display_name(item);
					if (short_name.size() > 8)
					{
						short_name = short_name.substr(0, 6) + "..";
					}
					clicked = ImGui::Button(short_name.c_str(), ImVec2(64.f, 56.f));
				}
			}
			else
			{
				clicked = ImGui::Button("+", ImVec2(64.f, 56.f));
			}
			const bool hovered       = ImGui::IsItemHovered();
			const bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			if (!hotkey.empty())
			{
				ImGui::TextDisabled("%s", hotkey.c_str());
			}
			else if (equipped)
			{
				ImGui::TextColored(accent, "Equipped");
			}
			else if (item)
			{
				std::string short_name = get_item_display_name(item);
				if (short_name.size() > 7)
				{
					short_name = short_name.substr(0, 6) + ".";
				}
				ImGui::TextDisabled("%s", short_name.c_str());
			}
			else
			{
				ImGui::TextDisabled("Empty");
			}

			if (hovered && item)
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(get_item_display_name(item).c_str());
				ImGui::Separator();
				ImGui::Text("Slot: %s %d", get_slot_kind_name(kind), num);
				const int stack = get_item_stack(item);
				if (stack > 1)
				{
					ImGui::Text("Stack: %d", stack);
				}
				const float durability     = get_item_durability(item);
				const float max_durability = get_item_max_durability(item);
				if (max_durability > 0.f)
				{
					ImGui::Text("Durability: %.0f / %.0f", durability, max_durability);
				}
				if (is_item_equipped(item))
				{
					ImGui::TextColored(accent, "Equipped");
				}
				ImGui::Separator();
				ImGui::TextDisabled("Left click: use or equip");
				ImGui::TextDisabled("Right click: return to inventory");
				ImGui::EndTooltip();
			}

			if (clicked)
			{
				if (item)
				{
					mgr.use_slot(kind, num);
				}
				else
				{
					mgr.m_selected_slot_x = c;
					mgr.m_selected_slot_y = r;
					mgr.m_show_picker     = true;
				}
			}
			if (item && right_clicked)
			{
				mgr.move_from_slot_to_inventory(c, r);
			}
			ImGui::EndGroup();
			ImGui::PopID();
		};

		auto draw_section = [&](const char* title, const char* subtitle, const std::vector<iVector2>& slots, float width = 0.f) {
			ImGui::BeginChild(title, ImVec2(width, 155.f), true);
			ImGui::TextUnformatted(title);
			if (subtitle && subtitle[0] != '\0')
			{
				ImGui::SameLine();
				ImGui::TextDisabled("(%s)", subtitle);
			}
			ImGui::Separator();
			bool first_slot = true;
			for (const auto& pos : slots)
			{
				if (!first_slot)
				{
					ImGui::SameLine(0.f, 8.f);
				}
				draw_slot(pos.x, pos.y);
				first_slot = false;
			}
			ImGui::EndChild();
		};

		draw_section("Armor & Accessories", "5 pieces", {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {1, 2}}, 395.f);
		ImGui::SameLine(0.f, 10.f);
		draw_section("Utility Trinkets", "Multi-Utility", {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, 0.f);

		draw_section("Food Buffs", "Shift+1..3", {{6, 0}, {6, 1}, {6, 2}}, 245.f);
		ImGui::SameLine(0.f, 10.f);
		draw_section("Ammunition", "Alt+R/T/Z", {{7, 0}, {7, 1}, {7, 2}}, 245.f);
		ImGui::SameLine(0.f, 10.f);
		draw_section("Keys & Tools", "2 slots", {{5, 0}, {5, 1}}, 0.f);

		draw_section("Quick Access", "Hotkeys Alt+1 through Alt+6", {{2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}, {7, 4}}, 0.f);

		// Item Picker Popup
		if (mgr.m_show_picker && mgr.m_selected_slot_x >= 0 && mgr.m_selected_slot_y >= 0)
		{
			slot_kind target_kind = get_slot_kind(mgr.m_selected_slot_x, mgr.m_selected_slot_y);
			ImGui::OpenPopup("Select Item to Place");

			if (ImGui::BeginPopupModal("Select Item to Place", &mgr.m_show_picker, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Select item for slot %s:", get_slot_kind_name(target_kind));
				ImGui::Separator();

				auto player_inv       = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
				static auto get_all_m = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");

				if (player_inv && get_all_m)
				{
					auto items_obj = mono::invoke_method(get_all_m, player_inv, nullptr);
					auto items     = unity::list_to_vector(items_obj);
					int count      = 0;

					for (auto* itm : items)
					{
						if (!itm)
						{
							continue;
						}
						if (fits_slot(target_kind, itm))
						{
							count++;
							std::string dname = get_item_display_name(itm);
							int stack         = get_item_stack(itm);
							std::string label = (stack > 1) ? std::format("{} (x{})", dname, stack) : dname;

							ImGui::PushID(itm);
							std::string prefab = get_item_prefab_name(itm);
							ImTextureID tex    = item_icons::get(prefab);
							if (tex)
							{
								ImGui::Image(tex, ImVec2(24, 24));
								ImGui::SameLine();
							}
							if (ImGui::Button(label.c_str()))
							{
								mgr.move_to_slot(player_inv, itm, mgr.m_selected_slot_x, mgr.m_selected_slot_y);
								mgr.m_show_picker = false;
								ImGui::CloseCurrentPopup();
							}
							ImGui::PopID();
						}
					}
					if (count == 0)
					{
						ImGui::TextDisabled("No compatible items found in main inventory.");
					}
				}

				ImGui::Separator();
				if (ImGui::Button("Cancel"))
				{
					mgr.m_show_picker = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		canvas::end_panel();
	}
}
