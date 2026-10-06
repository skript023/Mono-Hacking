#include "equipment_slots.hpp"
#include "hooking.hpp"
#include "menu_settings.hpp"
#include "notification/notification_service.hpp"
#include "unity/item_icons.hpp"
#include "unity/localization.hpp"
#include "utility/unity.hpp"

#include <algorithm>
#include <format>
#include <unordered_set>

namespace big
{
	namespace
	{
		// 8 columns x 5 rows grid layout according to BetrValheim v4:
		// Row 0: H  U1 U2 .  .  K1 F1 A1
		// Row 1: P  U3 U4 .  .  K2 F2 A2
		// Row 2: C  T  .  .  .  .  F3 A3
		// Row 3: L  .  .  .  .  .  .  .
		// Row 4: .  .  Q1 Q2 Q3 Q4 Q5 Q6
		const slot_kind s_layout[equipment_manager::grid_height][equipment_manager::grid_width] = {
		    {slot_kind::helmet, slot_kind::utility, slot_kind::utility, slot_kind::none, slot_kind::none, slot_kind::key, slot_kind::food, slot_kind::ammo},
		    {slot_kind::cape, slot_kind::utility, slot_kind::utility, slot_kind::none, slot_kind::none, slot_kind::key, slot_kind::food, slot_kind::ammo},
		    {slot_kind::chest, slot_kind::trinket, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::food, slot_kind::ammo},
		    {slot_kind::legs, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none, slot_kind::none},
		    {slot_kind::none, slot_kind::none, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick, slot_kind::quick}};

		std::string get_item_display_name(MonoObject* item_obj)
		{
			if (!item_obj)
				return "";
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item_obj);
			if (!shared)
				return "Unknown Item";
			auto raw_mono = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_name", MonoString*>(shared);
			if (!raw_mono)
				return "Unknown Item";
			std::string raw_name = mono::from_mono_string(raw_mono);
			if (raw_name.empty())
				return "Unknown Item";
			return localization::get_instance().localize(raw_name);
		}

		std::string get_item_prefab_name(MonoObject* item_obj)
		{
			if (!item_obj)
				return "";
			auto drop_prefab = mono::get_field_value<"ItemDrop/ItemData", "m_dropPrefab", MonoObject*>(item_obj);
			return drop_prefab ? unity::get_name(drop_prefab) : "";
		}

		float get_item_durability(MonoObject* item_obj)
		{
			if (!item_obj)
				return 0.f;
			return mono::get_field_value<"ItemDrop/ItemData", "m_durability", float>(item_obj);
		}

		float get_item_max_durability(MonoObject* item_obj)
		{
			if (!item_obj)
				return 1.f;
			static auto get_max_dur = mono::get_method("ItemDrop/ItemData", "GetMaxDurability", 0, "assembly_valheim");
			if (!get_max_dur)
				return 100.f;
			auto res = mono::invoke_method(get_max_dur, item_obj, nullptr);
			return res ? *reinterpret_cast<float*>(mono::object_unbox(res)) : 100.f;
		}

		int get_item_stack(MonoObject* item_obj)
		{
			if (!item_obj)
				return 0;
			return mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item_obj);
		}

		bool is_item_equipped(MonoObject* item_obj)
		{
			if (!item_obj)
				return false;
			return mono::get_field_value<"ItemDrop/ItemData", "m_equipped", bool>(item_obj);
		}
	}

	slot_kind equipment_manager::get_slot_kind(int x, int y)
	{
		if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
			return slot_kind::none;
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
			return 0;
		int count = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (s_layout[r][c] == kind)
				{
					count++;
					if (c == x && r == y)
						return count;
				}
			}
		}
		return 0;
	}

	iVector2 equipment_manager::get_slot_pos(slot_kind kind, int number)
	{
		if (kind == slot_kind::none || number <= 0)
			return {-1, -1};
		int count = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (s_layout[r][c] == kind)
				{
					count++;
					if (count == number)
						return {c, r};
				}
			}
		}
		return {-1, -1};
	}

	bool equipment_manager::fits_slot(slot_kind kind, MonoObject* item_data_obj)
	{
		if (!item_data_obj || kind == slot_kind::none)
			return false;
		if (kind == slot_kind::quick)
			return true;

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item_data_obj);
		if (!shared)
			return false;

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
			return {-1, -1};

		static auto get_item_at = mono::get_method("Inventory", "GetItemAt", 2, "assembly_valheim");
		if (!get_item_at)
			return {-1, -1};

		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				slot_kind kind = s_layout[r][c];
				if (kind != slot_kind::none && kind != slot_kind::quick && fits_slot(kind, item_data_obj))
				{
					int x = c, y = r;
					void* args[2] = {&x, &y};
					auto item_at = mono::invoke_method(get_item_at, equipment_inv, args);
					if (!item_at)
						return {c, r};
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
		m_initialized = false;
		m_cached_player = nullptr;
	}

	MonoObject* equipment_manager::create_page(int index)
	{
		auto inv_class = mono::get_class("Inventory", "assembly_valheim");
		if (!inv_class)
			return nullptr;

		auto inv_obj = mono::object_new(inv_class);
		if (!inv_obj)
			return nullptr;

		auto inv_ctor = mono::class_get_method_from_name(inv_class, ".ctor", 4);
		if (!inv_ctor)
			return nullptr;

		std::string title = (index == equipment_page_idx) ? "$ew_equipment_title" : "$ew_extra_title";
		auto title_mono = mono::to_mono_string(title);
		void* bkg = nullptr;
		int w = grid_width;
		int h = grid_height;
		void* args[4] = {title_mono, bkg, &w, &h};
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
					m_page_handles[i] = mono::retain(m_pages[i]);
			}
		}
	}

	void equipment_manager::init_or_sync()
	{
		if (m_syncing)
			return;

		auto local_player = unity::get_local_player();
		if (!local_player)
		{
			if (m_initialized)
				release_handles();
			return;
		}

		if (m_initialized && m_cached_player == local_player)
			return;

		m_syncing = true;
		release_handles();
		ensure_pages();
		m_cached_player = local_player;
		m_initialized = true;

		// Read player.m_customData["ew_extra_inventory"]
		auto custom_data = mono::get_field_value<"Player", "m_customData", MonoObject*>(local_player);
		if (custom_data)
		{
			auto dict_class = mono::object_get_class(custom_data);
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
				auto key_str = mono::to_mono_string("ew_extra_inventory");
				MonoString* val_out = nullptr;
				void* args[2] = {key_str, &val_out};
				auto res = mono::invoke_method(try_get_value, custom_data, args);
				bool found = res ? *reinterpret_cast<bool*>(mono::object_unbox(res)) : false;

				if (found && val_out && val_out->length > 0)
				{
					auto zpkg_class = mono::get_class("ZPackage", "assembly_valheim");
					if (zpkg_class)
					{
						auto pkg_obj = mono::object_new(zpkg_class);
						static auto pkg_ctor = mono::get_method_exact("ZPackage", ".ctor", {"string"}, "assembly_valheim");
						if (!pkg_ctor)
							pkg_ctor = mono::get_method_exact("ZPackage", ".ctor", {"System.String"}, "assembly_valheim");

						if (pkg_obj && pkg_ctor)
						{
							void* ctor_args[1] = {val_out};
							mono::invoke_method(pkg_ctor, pkg_obj, ctor_args);

							static auto read_int_m = mono::class_get_method_from_name(zpkg_class, "ReadInt", 0);
							if (read_int_m)
							{
								auto count_res = mono::invoke_method(read_int_m, pkg_obj, nullptr);
								int num_pages = count_res ? *reinterpret_cast<int*>(mono::object_unbox(count_res)) : 0;

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
			return;

		static auto equip_method = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
		if (!equip_method)
			return;

		m_in_equip = true;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				slot_kind kind = s_layout[r][c];
				auto item = get_item_at(c, r);
				if (!item)
					continue;

				// Wearable armor/cape/trinket/utility: ensure equipped
				if (kind == slot_kind::helmet || kind == slot_kind::cape || kind == slot_kind::chest ||
				    kind == slot_kind::legs || kind == slot_kind::trinket || kind == slot_kind::utility)
				{
					if (!is_item_equipped(item))
					{
						bool trigger = false;
						void* args[2] = {item, &trigger};
						mono::invoke_method(equip_method, humanoid, args);
					}
				}
				else
				{
					// Quick slot weapons or ammo that were saved as equipped
					if (is_item_equipped(item))
					{
						bool trigger = false;
						void* args[2] = {item, &trigger};
						mono::invoke_method(equip_method, humanoid, args);
					}
				}
			}
		}
		m_in_equip = false;
	}

	void equipment_manager::save_to_player()
	{
		if (!m_initialized || !m_cached_player || m_saving)
			return;

		m_saving = true;

		auto zpkg_class = mono::get_class("ZPackage", "assembly_valheim");
		if (!zpkg_class)
		{
			m_saving = false;
			return;
		}

		auto pkg_obj = mono::object_new(zpkg_class);
		auto pkg_ctor = mono::class_get_method_from_name(zpkg_class, ".ctor", 0);
		if (!pkg_obj || !pkg_ctor)
		{
			m_saving = false;
			return;
		}

		mono::invoke_method(pkg_ctor, pkg_obj, nullptr);

		static auto write_int_m = mono::get_method_exact("ZPackage", "Write", {"int"}, "assembly_valheim");
		if (!write_int_m)
			write_int_m = mono::get_method_exact("ZPackage", "Write", {"System.Int32"}, "assembly_valheim");
		if (!write_int_m)
			write_int_m = mono::get_method_overload("ZPackage", "Write", 1, nullptr, "int", "assembly_valheim");

		if (!write_int_m)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve ZPackage.Write(int)!";
			m_saving = false;
			return;
		}

		int num_pages = page_count;
		void* num_args[1] = {&num_pages};
		mono::invoke_method(write_int_m, pkg_obj, num_args);

		static auto save_method = mono::get_method("Inventory", "Save", 1, "assembly_valheim");
		if (!save_method)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve Inventory.Save!";
			m_saving = false;
			return;
		}

		for (int i = 0; i < page_count; ++i)
		{
			if (m_pages[i])
			{
				void* save_args[1] = {pkg_obj};
				mono::invoke_method(save_method, m_pages[i], save_args);
			}
		}

		static auto get_b64_m = mono::class_get_method_from_name(zpkg_class, "GetBase64", 0);
		if (!get_b64_m)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Failed to resolve ZPackage.GetBase64!";
			m_saving = false;
			return;
		}

		auto b64_obj = mono::invoke_method(get_b64_m, pkg_obj, nullptr);
		if (!b64_obj)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: GetBase64 returned null!";
			m_saving = false;
			return;
		}

		auto custom_data = mono::get_field_value<"Player", "m_customData", MonoObject*>(m_cached_player);
		if (!custom_data)
		{
			LOG(WARNING) << "[EquipmentSlots] save_to_player: Player.m_customData is null!";
			m_saving = false;
			return;
		}

		auto dict_class = mono::object_get_class(custom_data);
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
			m_saving = false;
			return;
		}

		auto key1 = mono::to_mono_string("ew_extra_inventory");
		void* args1[2] = {key1, b64_obj};
		mono::invoke_method(set_item_m, custom_data, args1);

		auto key2 = mono::to_mono_string("ew_equipment_layout");
		auto val2 = mono::to_mono_string("4");
		void* args2[2] = {key2, val2};
		mono::invoke_method(set_item_m, custom_data, args2);

		LOG(INFO) << "[EquipmentSlots] Successfully saved equipment pages to player customData!";
		m_saving = false;
	}

	MonoObject* equipment_manager::get_equipment_inventory()
	{
		ensure_pages();
		return m_pages[equipment_page_idx];
	}

	MonoObject* equipment_manager::get_item_at(int x, int y)
	{
		if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
			return nullptr;

		auto eq = get_equipment_inventory();
		if (!eq)
			return nullptr;

		static auto get_item_at_m = mono::get_method("Inventory", "GetItemAt", 2, "assembly_valheim");
		if (!get_item_at_m)
			return nullptr;

		void* args[2] = {&x, &y};
		return mono::invoke_method(get_item_at_m, eq, args);
	}

	bool equipment_manager::move_to_slot(MonoObject* from_inv, MonoObject* item, int to_x, int to_y)
	{
		auto eq = get_equipment_inventory();
		if (!eq || !from_inv || !item)
			return false;

		slot_kind kind = get_slot_kind(to_x, to_y);
		if (!fits_slot(kind, item))
			return false;

		static auto move_method = mono::get_method("Inventory", "MoveItemToThis", 5, "assembly_valheim");
		if (!move_method)
			return false;

		int stack = get_item_stack(item);
		void* args[5] = {from_inv, item, &stack, &to_x, &to_y};
		auto res = mono::invoke_method(move_method, eq, args);
		bool success = res ? *reinterpret_cast<bool*>(mono::object_unbox(res)) : false;

		if (success && m_cached_player)
		{
			if (kind == slot_kind::helmet || kind == slot_kind::cape || kind == slot_kind::chest ||
			    kind == slot_kind::legs || kind == slot_kind::trinket || kind == slot_kind::utility)
			{
				m_in_equip = true;
				static auto equip_m = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
				if (equip_m)
				{
					bool trigger = true;
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
			return false;

		auto item = get_item_at(from_x, from_y);
		if (!item)
			return false;

		if (is_item_equipped(item))
		{
			m_in_equip = true;
			static auto unequip_m = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
			if (unequip_m)
			{
				bool trigger = true;
				void* uq_args[2] = {item, &trigger};
				mono::invoke_method(unequip_m, m_cached_player, uq_args);
			}
			m_in_equip = false;
		}

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
		if (!player_inv)
			return false;

		static auto move_m = mono::get_method("Inventory", "MoveItemToThis", 2, "assembly_valheim");
		if (!move_m)
			return false;

		void* args[2] = {eq, item};
		mono::invoke_method(move_m, player_inv, args);
		save_to_player();
		return true;
	}

	void equipment_manager::use_slot(slot_kind kind, int number)
	{
		auto pos = get_slot_pos(kind, number);
		if (pos.x < 0 || pos.y < 0)
			return;

		auto eq = get_equipment_inventory();
		auto item = get_item_at(pos.x, pos.y);
		if (!eq || !item || !m_cached_player)
			return;

		if (kind == slot_kind::ammo)
		{
			static auto equip_m = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
			if (equip_m)
			{
				bool trigger = true;
				void* args[2] = {item, &trigger};
				mono::invoke_method(equip_m, m_cached_player, args);
				notification::info("Ammo Slot", std::format("Equipped {} (Slot {})", get_item_display_name(item), number));
			}
		}
		else
		{
			auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
			static auto toggle_equipped_m = mono::get_method("Humanoid", "ToggleEquipped", 1, "assembly_valheim");
			static auto use_m = mono::get_method("Humanoid", "UseItem", 3, "assembly_valheim");

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
		auto eq = get_equipment_inventory();
		if (!player || !eq)
			return;

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
		if (!player_inv)
			return;

		static auto get_all_m = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");
		if (!get_all_m)
			return;

		auto all_items = mono::invoke_method(get_all_m, player_inv, nullptr);
		if (!all_items)
			return;

		auto items_vec = unity::list_to_vector(all_items);
		int moved_count = 0;
		std::unordered_set<MonoObject*> moved_items;

		// Pass 1: Prioritize currently equipped items so worn gear moves into dedicated slots first
		for (auto* itm : items_vec)
		{
			if (!itm || !is_item_equipped(itm))
				continue;

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
				continue;

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
			notification::success("Equipment Slots", std::format("Auto-filled {} items into dedicated slots!", moved_count));
		else
			notification::info("Equipment Slots", "No eligible items to auto-fill.");
	}

	void equipment_manager::deposit_all_to_inventory()
	{
		auto player = unity::get_local_player();
		auto eq = get_equipment_inventory();
		if (!player || !eq)
			return;

		int deposited = 0;
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (get_item_at(c, r))
				{
					if (move_from_slot_to_inventory(c, r))
						deposited++;
				}
			}
		}
		if (deposited > 0)
			notification::info("Equipment Slots", std::format("Deposited {} items back to inventory.", deposited));
	}

	void equipment_manager::unequip_all()
	{
		if (!m_cached_player)
			return;

		static auto unequip_all_m = mono::get_method("Humanoid", "UnequipAllItems", 0, "assembly_valheim");
		if (unequip_all_m)
			mono::invoke_method(unequip_all_m, m_cached_player, nullptr);

		for (auto* util : m_extra_utilities)
		{
			if (util)
				mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(util, false);
		}
		m_extra_utilities.clear();
	}

	void equipment_manager::tick(MonoObject* player)
	{
		if (!g_settings.self.equipment_slots_enabled)
			return;

		init_or_sync();
		if (!m_initialized || player != m_cached_player)
			return;

		// Hotkey listener for Ammo switching (Alt + R, Alt + T, Alt + Z)
		if (g_settings.self.equipment_slots_hotkeys)
		{
			bool alt_down = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			if (alt_down)
			{
				bool r_down = (GetAsyncKeyState('R') & 0x8000) != 0;
				if (r_down && !m_r_key_down)
					use_slot(slot_kind::ammo, 1);
				m_r_key_down = r_down;

				bool t_down = (GetAsyncKeyState('T') & 0x8000) != 0;
				if (t_down && !m_t_key_down)
					use_slot(slot_kind::ammo, 2);
				m_t_key_down = t_down;

				bool z_down = ((GetAsyncKeyState('Z') & 0x8000) != 0) || ((GetAsyncKeyState('Y') & 0x8000) != 0);
				if (z_down && !m_z_key_down)
					use_slot(slot_kind::ammo, 3);
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
			return;

		auto eq = get_equipment_inventory();
		if (!eq)
			return;

		auto home = find_home_slot(eq, item);
		if (home.x < 0 || home.y < 0)
			return;

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(m_cached_player);
		if (!player_inv)
			return;

		auto existing = get_item_at(home.x, home.y);
		if (existing == item)
			return;

		move_to_slot(player_inv, item, home.x, home.y);
	}

	bool equipment_manager::handle_inventory_equip(MonoObject* humanoid, MonoObject* item)
	{
		if (!g_settings.self.equipment_slots_enabled || m_in_equip || !m_initialized)
			return false;

		if (!humanoid || humanoid != m_cached_player || !item)
			return false;

		auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(humanoid);
		if (!player_inv)
			return false;

		auto eq = get_equipment_inventory();
		if (!eq)
			return false;

		// Verify that this item is in the main player inventory
		if (!detour_base::get_original<hooks::inventory_contains_item>()(player_inv, item))
			return false;

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
		if (!shared)
			return false;

		int item_type = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared);

		iVector2 target_slot = {-1, -1};
		if (item_type == 6)
			target_slot = {0, 0}; // Helmet
		else if (item_type == 17)
			target_slot = {0, 1}; // Cape
		else if (item_type == 7)
			target_slot = {0, 2}; // Chest
		else if (item_type == 11)
			target_slot = {0, 3}; // Legs
		else if (item_type == 24)
			target_slot = {1, 2}; // Trinket
		else if (item_type == 18)     // Utility
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
					target_slot = util_slots[0];
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

		static auto move_5_m = mono::get_method("Inventory", "MoveItemToThis", 5, "assembly_valheim");
		static auto move_2_m = mono::get_method("Inventory", "MoveItemToThis", 2, "assembly_valheim");
		static auto unequip_m = mono::get_method("Humanoid", "UnequipItem", 2, "assembly_valheim");
		static auto equip_m = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");

		auto existing = get_item_at(target_slot.x, target_slot.y);
		if (existing)
		{
			// 1. Unequip existing item in that slot
			if (is_item_equipped(existing) && unequip_m)
			{
				bool trigger = true;
				void* uq_args[2] = {existing, &trigger};
				mono::invoke_method(unequip_m, humanoid, uq_args);
			}

			// 2. Move existing to temp slot (3, 0) in eq
			if (move_5_m)
			{
				int temp_x = 3, temp_y = 0;
				int stack = get_item_stack(existing);
				void* args1[5] = {eq, existing, &stack, &temp_x, &temp_y};
				mono::invoke_method(move_5_m, eq, args1);
			}

			// 3. Move new item from player_inv to target_slot in eq
			if (move_5_m)
			{
				int stack = get_item_stack(item);
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
				int stack = get_item_stack(item);
				void* args[5] = {player_inv, item, &stack, &target_slot.x, &target_slot.y};
				mono::invoke_method(move_5_m, eq, args);
			}
		}

		// 5. Equip new item
		if (equip_m)
		{
			bool trigger = true;
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
			return false;

		auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
		if (!shared)
			return false;

		int item_type = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_itemType", int>(shared);
		if (item_type != 18) // ItemType.Utility
			return false;

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
			return false;

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
						bool quiet = false;
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
			return;

		auto seman = mono::get_field_value<"Character", "m_seman", MonoObject*>(humanoid);
		auto equip_effects = mono::get_field_value<"Humanoid", "m_equipmentStatusEffects", MonoObject*>(humanoid);
		if (!seman)
			return;

		static auto have_se_m = mono::get_method_exact("SEMan", "HaveStatusEffect", {"StatusEffect"}, "assembly_valheim");
		static auto add_se_m = mono::get_method_exact("SEMan", "AddStatusEffect", {"StatusEffect", "System.Boolean", "System.Int32", "System.Single", "System.Int16"}, "assembly_valheim");

		for (auto* util : m_extra_utilities)
		{
			if (!util)
				continue;
			auto shared = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(util);
			if (!shared)
				continue;

			auto eq_se = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_equipStatusEffect", MonoObject*>(shared);
			if (eq_se && have_se_m && add_se_m)
			{
				void* have_args[1] = {eq_se};
				auto have_res = mono::invoke_method(have_se_m, seman, have_args);
				bool has = have_res ? *reinterpret_cast<bool*>(mono::object_unbox(have_res)) : false;

				if (!has)
				{
					bool reset_time = false;
					int item_lvl = 0;
					float skill_lvl = 0.f;
					int16_t variant = -1;
					void* add_args[5] = {eq_se, &reset_time, &item_lvl, &skill_lvl, &variant};
					mono::invoke_method(add_se_m, seman, add_args);
				}

				if (equip_effects)
				{
					auto hs_class = mono::object_get_class(equip_effects);
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
			return;
		for (auto* util : m_extra_utilities)
		{
			if (util)
				mono::set_field_value<"ItemDrop/ItemData", "m_equipped">(util, false);
		}
		m_extra_utilities.clear();
	}

	void equipment_manager::on_player_save(MonoObject* player, MonoObject* pkg)
	{
		if (player == m_cached_player)
			save_to_player();
	}

	void equipment_manager::draw_ui()
	{
		if (!g_settings.self.equipment_slots_enabled || !g_settings.self.equipment_slots_window)
			return;

		auto& mgr = get();
		mgr.init_or_sync();

		auto player = unity::get_local_player();
		if (!player)
			return;

		ImGui::SetNextWindowSize(ImVec2(620, 520), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("Equipment & Quick Slots (BetrValheim)", &g_settings.self.equipment_slots_window, ImGuiWindowFlags_NoCollapse))
		{
			ImGui::End();
			return;
		}

		// Action Toolbar
		if (ImGui::Button("Auto-Fill from Inventory"))
			mgr.auto_fill_from_inventory();
		ImGui::SameLine();
		if (ImGui::Button("Deposit All to Inventory"))
			mgr.deposit_all_to_inventory();
		ImGui::SameLine();
		if (ImGui::Button("Save Slots"))
		{
			mgr.save_to_player();
			notification::success("Equipment Slots", "Saved equipment slots to character!");
		}
		ImGui::SameLine();
		ImGui::Checkbox("Hotkeys Enabled", &g_settings.self.equipment_slots_hotkeys);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.f), "Alt+1..6: Quick Slots | Shift+1..3: Eat Food | Alt+R/T/Z: Select Ammo");
		ImGui::Separator();

		const float cell_size = 62.f;

		// Draw BetrValheim 8x5 Grid
		for (int r = 0; r < grid_height; ++r)
		{
			for (int c = 0; c < grid_width; ++c)
			{
				if (c > 0)
					ImGui::SameLine();

				slot_kind kind = s_layout[r][c];
				int num = get_slot_number(kind, c, r);
				auto item = mgr.get_item_at(c, r);

				ImGui::PushID(r * grid_width + c);

				// Slot background colors
				ImVec4 border_col = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
				ImVec4 bg_col = ImVec4(0.12f, 0.12f, 0.16f, 0.8f);

				switch (kind)
				{
				case slot_kind::helmet:
				case slot_kind::cape:
				case slot_kind::chest:
				case slot_kind::legs:
					border_col = ImVec4(0.2f, 0.6f, 1.0f, 0.8f);
					bg_col = ImVec4(0.1f, 0.2f, 0.35f, 0.7f);
					break;
				case slot_kind::utility:
				case slot_kind::trinket:
					border_col = ImVec4(0.7f, 0.3f, 0.9f, 0.8f);
					bg_col = ImVec4(0.25f, 0.1f, 0.35f, 0.7f);
					break;
				case slot_kind::food:
					border_col = ImVec4(1.0f, 0.6f, 0.1f, 0.8f);
					bg_col = ImVec4(0.35f, 0.2f, 0.05f, 0.7f);
					break;
				case slot_kind::ammo:
					border_col = ImVec4(1.0f, 0.9f, 0.2f, 0.8f);
					bg_col = ImVec4(0.35f, 0.3f, 0.05f, 0.7f);
					break;
				case slot_kind::key:
					border_col = ImVec4(0.9f, 0.8f, 0.5f, 0.8f);
					bg_col = ImVec4(0.3f, 0.25f, 0.15f, 0.7f);
					break;
				case slot_kind::quick:
					border_col = ImVec4(0.2f, 0.9f, 0.5f, 0.8f);
					bg_col = ImVec4(0.1f, 0.3f, 0.2f, 0.7f);
					break;
				default:
					border_col = ImVec4(0.2f, 0.2f, 0.2f, 0.2f);
					bg_col = ImVec4(0.08f, 0.08f, 0.08f, 0.3f);
					break;
				}

				ImGui::PushStyleColor(ImGuiCol_Border, border_col);
				ImGui::PushStyleColor(ImGuiCol_Button, bg_col);

				bool clicked = false;
				std::string btn_label = "";

				if (kind == slot_kind::none)
				{
					ImGui::Dummy(ImVec2(cell_size, cell_size));
				}
				else
				{
					if (item)
					{
						std::string prefab = get_item_prefab_name(item);
						ImTextureID tex = item_icons::get(prefab);
						if (tex)
						{
							clicked = ImGui::ImageButton("##slot_icon", tex, ImVec2(cell_size - 12.f, cell_size - 12.f));
						}
						else
						{
							std::string short_name = get_item_display_name(item);
							if (short_name.size() > 7)
								short_name = short_name.substr(0, 6) + "..";
							clicked = ImGui::Button(short_name.c_str(), ImVec2(cell_size, cell_size));
						}
					}
					else
					{
						std::string kind_tag;
						switch (kind)
						{
						case slot_kind::helmet: kind_tag = "Helm"; break;
						case slot_kind::cape: kind_tag = "Cape"; break;
						case slot_kind::chest: kind_tag = "Chest"; break;
						case slot_kind::legs: kind_tag = "Legs"; break;
						case slot_kind::trinket: kind_tag = "Trink"; break;
						case slot_kind::utility: kind_tag = std::format("Util{}", num); break;
						case slot_kind::food: kind_tag = std::format("Food{}", num); break;
						case slot_kind::ammo: kind_tag = std::format("Ammo{}", num); break;
						case slot_kind::key: kind_tag = std::format("Key{}", num); break;
						case slot_kind::quick: kind_tag = std::format("Q{}", num); break;
						default: kind_tag = ""; break;
						}
						clicked = ImGui::Button(kind_tag.c_str(), ImVec2(cell_size, cell_size));
					}

					// Hover tooltip
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						if (item)
						{
							ImGui::TextUnformatted(get_item_display_name(item).c_str());
							ImGui::Separator();
							ImGui::Text("Slot: %s %d", get_slot_kind_name(kind), num);
							int stack = get_item_stack(item);
							if (stack > 1)
								ImGui::Text("Stack: %d", stack);
							float dur = get_item_durability(item);
							float max_dur = get_item_max_durability(item);
							if (max_dur > 0.f)
								ImGui::Text("Durability: %.0f / %.0f", dur, max_dur);
							if (is_item_equipped(item))
								ImGui::TextColored(ImVec4(0.2f, 1.f, 0.2f, 1.f), "[Equipped]");
							ImGui::Separator();
							ImGui::TextDisabled("Left-Click: Use/Equip | Right-Click: Deposit to Inventory");
						}
						else
						{
							ImGui::Text("Empty %s Slot (#%d)", get_slot_kind_name(kind), num);
							if (kind == slot_kind::food)
								ImGui::TextColored(ImVec4(1.f, 0.8f, 0.2f, 1.f), "Hotkey: Shift + %d", num);
							else if (kind == slot_kind::quick)
								ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Hotkey: Alt + %d", num);
							else if (kind == slot_kind::ammo)
							{
								const char* ammo_key = (num == 1) ? "R" : (num == 2) ? "T" : "Z";
								ImGui::TextColored(ImVec4(1.f, 0.9f, 0.2f, 1.f), "Hotkey: Alt + %s", ammo_key);
							}
							ImGui::Separator();
							ImGui::TextDisabled("Click to choose an item from inventory to place here.");
						}
						ImGui::EndTooltip();
					}

					// Left click action
					if (clicked)
					{
						if (item)
						{
							// Use / Equip
							mgr.use_slot(kind, num);
						}
						else
						{
							// Open item picker for this slot
							mgr.m_selected_slot_x = c;
							mgr.m_selected_slot_y = r;
							mgr.m_show_picker = true;
						}
					}

					// Right click action: return item to inventory
					if (item && ImGui::IsItemClicked(ImGuiMouseButton_Right))
					{
						mgr.move_from_slot_to_inventory(c, r);
					}
				}

				ImGui::PopStyleColor(2);
				ImGui::PopID();
			}
		}

		// Item Picker Popup
		if (mgr.m_show_picker && mgr.m_selected_slot_x >= 0 && mgr.m_selected_slot_y >= 0)
		{
			slot_kind target_kind = get_slot_kind(mgr.m_selected_slot_x, mgr.m_selected_slot_y);
			ImGui::OpenPopup("Select Item to Place");

			if (ImGui::BeginPopupModal("Select Item to Place", &mgr.m_show_picker, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Select item for slot %s:", get_slot_kind_name(target_kind));
				ImGui::Separator();

				auto player_inv = mono::get_field_value<"Humanoid", "m_inventory", MonoObject*>(player);
				static auto get_all_m = mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim");

				if (player_inv && get_all_m)
				{
					auto items_obj = mono::invoke_method(get_all_m, player_inv, nullptr);
					auto items = unity::list_to_vector(items_obj);
					int count = 0;

					for (auto* itm : items)
					{
						if (!itm)
							continue;
						if (fits_slot(target_kind, itm))
						{
							count++;
							std::string dname = get_item_display_name(itm);
							int stack = get_item_stack(itm);
							std::string label = (stack > 1) ? std::format("{} (x{})", dname, stack) : dname;

							ImGui::PushID(itm);
							std::string prefab = get_item_prefab_name(itm);
							ImTextureID tex = item_icons::get(prefab);
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

		ImGui::End();
	}
}

