#include "item_spawner.hpp"
#include "item_icons.hpp"
#include "unity/item_data.hpp"
#include "unity/localization.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include "notification/notification_service.hpp"
#include "astra/host/canvas.hpp"
#include "fiber_pool.hpp"
#include "script.hpp"

#include <algorithm>
#include <atomic>
#include <format>
#include <imgui.h>
#include <unordered_map>
#include <unordered_set>

namespace big
{
	namespace
	{
		const std::vector<item_spawner::item_entry> default_catalog = {
		    // Weapons - Swords & Blades
		    {"SwordMistwalker", "Mistwalker", "Weapons", 1, 4},
		    {"SwordBlackmetal", "Blackmetal Sword", "Weapons", 1, 4},
		    {"SwordSilver", "Silver Sword", "Weapons", 1, 4},
		    {"SwordIron", "Iron Sword", "Weapons", 1, 4},
		    {"SwordDyrnwyn", "Dyrnwyn (Fire Sword)", "Weapons", 1, 4},
		    {"SwordNidhogg", "Nidhoggr", "Weapons", 1, 4},
		    {"AxeBlackMetal", "Blackmetal Axe", "Weapons", 1, 4},
		    {"AxeJotunBane", "Jotun Bane", "Weapons", 1, 4},
		    {"AxeBerserkir", "Berserkir Axes", "Weapons", 1, 4},
		    {"BattleaxeCrystal", "Crystal Battleaxe", "Weapons", 1, 4},
		    {"KnifeSkollAndHati", "Skoll and Hati", "Weapons", 1, 4},
		    {"KnifeBlackMetal", "Blackmetal Knife", "Weapons", 1, 4},
		    {"KnifeSilver", "Silver Knife", "Weapons", 1, 4},

		    // Weapons - Blunt & Polearms
		    {"MaceSilver", "Frostner", "Weapons", 1, 4},
		    {"MaceNeedle", "Porcupine", "Weapons", 1, 4},
		    {"MaceIron", "Iron Mace", "Weapons", 1, 4},
		    {"SledgeDemolisher", "Demolisher", "Weapons", 1, 4},
		    {"SledgeIron", "Iron Sledge", "Weapons", 1, 4},
		    {"AtgeirHimminAfl", "Himmin Afl", "Weapons", 1, 4},
		    {"AtgeirBlackmetal", "Blackmetal Atgeir", "Weapons", 1, 4},
		    {"AtgeirIron", "Iron Atgeir", "Weapons", 1, 4},
		    {"SpearCarapace", "Carapace Spear", "Weapons", 1, 4},

		    // Weapons - Bows & Crossbows
		    {"BowAshlands", "Ashlands Bow", "Weapons", 1, 4},
		    {"BowSpineSnap", "Spine Snap", "Weapons", 1, 4},
		    {"BowDraugrFang", "Draugr Fang", "Weapons", 1, 4},
		    {"BowHuntsman", "Huntsman Bow", "Weapons", 1, 4},
		    {"CrossbowArbalest", "Arbalest", "Weapons", 1, 4},
		    {"CrossbowRipper", "Ripper", "Weapons", 1, 4},

		    // Weapons - Magic & Staves
		    {"StaffFireball", "Staff of Embers", "Weapons", 1, 4},
		    {"StaffIceShards", "Staff of Frost", "Weapons", 1, 4},
		    {"StaffShield", "Staff of Protection", "Weapons", 1, 4},
		    {"StaffSkeleton", "Dead Raiser", "Weapons", 1, 4},
		    {"StaffLightning", "Staff of the Wild", "Weapons", 1, 4},
		    {"StaffRedDumortierite", "Staff of Fracturing", "Weapons", 1, 4},

		    // Armor & Gear
		    {"CapeFeather", "Feather Cape", "Armor & Gear", 1, 4},
		    {"CapeLox", "Lox Cape", "Armor & Gear", 1, 4},
		    {"CapeWolf", "Wolf Fur Cape", "Armor & Gear", 1, 4},
		    {"CapeAsh", "Ashlands Cape", "Armor & Gear", 1, 4},
		    {"HelmetCarapace", "Carapace Helmet", "Armor & Gear", 1, 4},
		    {"ArmorCarapaceChest", "Carapace Breastplate", "Armor & Gear", 1, 4},
		    {"ArmorCarapaceLegs", "Carapace Greaves", "Armor & Gear", 1, 4},
		    {"HelmetAshlandsMedium", "Flametal Helmet", "Armor & Gear", 1, 4},
		    {"ArmorAshlandsMediumChest", "Flametal Armor", "Armor & Gear", 1, 4},
		    {"ArmorAshlandsMediumLegs", "Flametal Greaves", "Armor & Gear", 1, 4},
		    {"HelmetMage", "Eitr-weave Hood", "Armor & Gear", 1, 4},
		    {"ArmorMageChest", "Eitr-weave Robe", "Armor & Gear", 1, 4},
		    {"ArmorMageLegs", "Eitr-weave Trousers", "Armor & Gear", 1, 4},
		    {"HelmetPadded", "Padded Helmet", "Armor & Gear", 1, 4},
		    {"ArmorPaddedChest", "Padded Cuirass", "Armor & Gear", 1, 4},
		    {"ArmorPaddedGreaves", "Padded Greaves", "Armor & Gear", 1, 4},
		    {"HelmetDrake", "Drake Helmet", "Armor & Gear", 1, 4},
		    {"ArmorWolfChest", "Wolf Armor Chest", "Armor & Gear", 1, 4},
		    {"ArmorWolfLegs", "Wolf Armor Legs", "Armor & Gear", 1, 4},
		    {"ShieldCarapace", "Carapace Shield", "Armor & Gear", 1, 4},
		    {"ShieldBlackmetal", "Blackmetal Shield", "Armor & Gear", 1, 4},
		    {"ShieldSilver", "Silver Shield", "Armor & Gear", 1, 4},
		    {"ShieldFlametal", "Flametal Shield", "Armor & Gear", 1, 4},

		    // Food & Potions
		    {"MeatPlatter", "Meat Platter", "Food & Potions", 10, 1},
		    {"MisthareSupreme", "Misthare Supreme", "Food & Potions", 10, 1},
		    {"FishAndBread", "Fish 'n' Bread", "Food & Potions", 10, 1},
		    {"Salad", "Salad", "Food & Potions", 10, 1},
		    {"MushroomOmelette", "Mushroom Omelette", "Food & Potions", 10, 1},
		    {"BloodPudding", "Blood Pudding", "Food & Potions", 10, 1},
		    {"LoxPie", "Lox Meat Pie", "Food & Potions", 10, 1},
		    {"SerpentStew", "Serpent Stew", "Food & Potions", 10, 1},
		    {"Sausages", "Sausages", "Food & Potions", 20, 1},
		    {"MagicallyStuffedMushroom", "Stuffed Mushroom", "Food & Potions", 10, 1},
		    {"YggdrasilPorridge", "Yggdrasil Porridge", "Food & Potions", 10, 1},
		    {"PiquantPie", "Piquant Pie", "Food & Potions", 10, 1},
		    {"SizzlingBerryTart", "Sizzling Berry Tart", "Food & Potions", 10, 1},
		    {"MeadHealthMajor", "Major Healing Mead", "Food & Potions", 10, 1},
		    {"MeadHealthMedium", "Medium Healing Mead", "Food & Potions", 10, 1},
		    {"MeadStaminaLingering", "Lingering Stamina Mead", "Food & Potions", 10, 1},
		    {"MeadStaminaMedium", "Medium Stamina Mead", "Food & Potions", 10, 1},
		    {"MeadEitrMinor", "Minor Eitr Mead", "Food & Potions", 10, 1},
		    {"MeadFrostResist", "Frost Resistance Mead", "Food & Potions", 10, 1},
		    {"MeadPoisonResist", "Poison Resistance Mead", "Food & Potions", 10, 1},

		    // Materials
		    {"Iron", "Iron", "Materials", 30, 1},
		    {"Silver", "Silver", "Materials", 30, 1},
		    {"BlackMetal", "Black Metal", "Materials", 30, 1},
		    {"FlametalNew", "Flametal", "Materials", 30, 1},
		    {"Bronze", "Bronze", "Materials", 30, 1},
		    {"Copper", "Copper", "Materials", 30, 1},
		    {"Tin", "Tin", "Materials", 30, 1},
		    {"Eitr", "Refined Eitr", "Materials", 50, 1},
		    {"Softtissue", "Soft Tissue", "Materials", 50, 1},
		    {"BlackCore", "Black Core", "Materials", 20, 1},
		    {"DvergrNeedle", "Dvergr Extractor", "Materials", 10, 1},
		    {"YggdrasilWood", "Yggdrasil Wood", "Materials", 50, 1},
		    {"BlackMarble", "Black Marble", "Materials", 50, 1},
		    {"Ashwood", "Ashwood", "Materials", 50, 1},
		    {"Grausten", "Grausten", "Materials", 50, 1},
		    {"Wood", "Wood", "Materials", 50, 1},
		    {"FineWood", "Fine Wood", "Materials", 50, 1},
		    {"RoundLog", "Core Wood", "Materials", 50, 1},
		    {"Stone", "Stone", "Materials", 50, 1},
		    {"Chitin", "Chitin", "Materials", 50, 1},
		    {"Chain", "Chain", "Materials", 50, 1},
		    {"Tar", "Tar", "Materials", 50, 1},
		    {"LinenThread", "Linen Thread", "Materials", 50, 1},
		    {"SerpentScale", "Serpent Scale", "Materials", 50, 1},
		    {"DragonTear", "Dragon Tear", "Materials", 10, 1},
		    {"Wishbone", "Wishbone", "Materials", 1, 1},
		    {"CryptKey", "Swamp Key", "Materials", 1, 1},
		    {"HardAntler", "Hard Antler", "Materials", 10, 1},

		    // Tools & Utilities
		    {"PickaxeBlackMetal", "Black Metal Pickaxe", "Tools", 1, 4},
		    {"PickaxeIron", "Iron Pickaxe", "Tools", 1, 4},
		    {"Hammer", "Hammer", "Tools", 1, 3},
		    {"Hoe", "Hoe", "Tools", 1, 3},
		    {"Cultivator", "Cultivator", "Tools", 1, 3},
		    {"FishingRod", "Fishing Rod", "Tools", 1, 1},
		    {"FishingBait", "Fishing Bait", "Tools", 100, 1},
		    {"BeltStrength", "Megingjord", "Tools", 1, 1},
		    {"Demister", "Wisplight", "Tools", 1, 1},

		    // Ammo
		    {"ArrowCarapace", "Carapace Arrow", "Ammo", 100, 1},
		    {"ArrowNeedle", "Needle Arrow", "Ammo", 100, 1},
		    {"ArrowObsidian", "Obsidian Arrow", "Ammo", 100, 1},
		    {"ArrowFrost", "Frost Arrow", "Ammo", 100, 1},
		    {"ArrowPoison", "Poison Arrow", "Ammo", 100, 1},
		    {"ArrowSilver", "Silver Arrow", "Ammo", 100, 1},
		    {"ArrowIron", "Iron Arrow", "Ammo", 100, 1},
		    {"BoltCarapace", "Carapace Bolt", "Ammo", 100, 1},
		    {"BoltBlackmetal", "Blackmetal Bolt", "Ammo", 100, 1},
		    {"BoltIron", "Iron Bolt", "Ammo", 100, 1}};
	}

	void item_spawner::initialize_impl()
	{
		std::lock_guard lock(g_items_mutex);
		if (g_initialized && !g_items.empty())
			return;

		g_items = default_catalog;
		g_initialized = true;
	}

	bool item_spawner::is_refreshing_impl()
	{
		return s_is_refreshing.load();
	}

	void item_spawner::scan_worker_impl()
	{
		TRY_CLAUSE
		{
			item_spawner::initialize();

			auto obj_db = unity::get_object_db();
			if (!obj_db)
			{
				notification::warning("Item Spawner", "ObjectDB is not loaded yet. Join a world first!");
				s_is_refreshing = false;
				return;
			}

			auto obj_db_class = mono::get_class("ObjectDB", "assembly_valheim");
			if (!obj_db_class)
			{
				s_is_refreshing = false;
				return;
			}

			auto m_items_field = mono::get_field(obj_db_class, "m_items");
			if (!m_items_field)
			{
				s_is_refreshing = false;
				return;
			}

			MonoObject* list_obj = nullptr;
			mono::get_field_value(obj_db, m_items_field, &list_obj);
			if (!list_obj)
			{
				s_is_refreshing = false;
				return;
			}

			auto item_prefabs = unity::list_to_vector(list_obj);
			if (item_prefabs.empty())
			{
				s_is_refreshing = false;
				return;
			}

			std::vector<item_spawner::item_entry> scanned;
			scanned.reserve(item_prefabs.size());
			std::unordered_set<std::string> seen;
			std::unordered_map<std::string, std::string> loc_cache;

			auto loc = localization::get_instance();
			auto item_drop_class = mono::get_class("ItemDrop", "assembly_valheim");
			auto m_item_data_field = item_drop_class ? mono::get_field(item_drop_class, "m_itemData") : nullptr;
			auto item_data_class = mono::get_class("ItemDrop/ItemData", "assembly_valheim");
			auto m_shared_field = item_data_class ? mono::get_field(item_data_class, "m_shared") : nullptr;
			auto shared_class = mono::get_class("ItemDrop/ItemData/SharedData", "assembly_valheim");
			auto m_name_field = shared_class ? mono::get_field(shared_class, "m_name") : nullptr;
			auto m_type_field = shared_class ? mono::get_field(shared_class, "m_itemType") : nullptr;
			auto m_stack_field = shared_class ? mono::get_field(shared_class, "m_maxStackSize") : nullptr;
			auto m_quality_field = shared_class ? mono::get_field(shared_class, "m_maxQuality") : nullptr;

			static auto comp_method = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (!comp_method)
				comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");

			auto mono_type = item_drop_class ? mono::reflection_type(item_drop_class) : nullptr;

			for (auto* prefab : item_prefabs)
			{
				if (!prefab)
					continue;

				std::string p_name = unity::get_name(prefab);
				if (p_name.empty() || p_name == "unknown" || seen.contains(p_name))
					continue;

				seen.insert(p_name);
				item_spawner::item_entry entry;
				entry.prefab_name = p_name;
				entry.display_name = p_name;
				entry.category = "Misc";
				entry.max_stack = 50;
				entry.max_quality = 1;

				// Check ItemDrop
				if (comp_method && mono_type)
				{
					void* args[1] = {mono_type};
					auto drop_comp = mono::invoke_method(comp_method, prefab, args);
					if (drop_comp && m_item_data_field && m_shared_field)
					{
						MonoObject* item_data_obj = nullptr;
						mono::get_field_value(drop_comp, m_item_data_field, &item_data_obj);
						if (item_data_obj)
						{
							MonoObject* shared_obj = nullptr;
							mono::get_field_value(item_data_obj, m_shared_field, &shared_obj);
							if (shared_obj)
							{
								if (m_name_field)
								{
									MonoString* raw_name = nullptr;
									mono::get_field_value(shared_obj, m_name_field, &raw_name);
									if (raw_name && raw_name->length > 0)
									{
										if (raw_name->chars[0] == '$')
										{
											std::string raw_str = mono::from_mono_string(raw_name);
											if (auto it = loc_cache.find(raw_str); it != loc_cache.end())
											{
												entry.display_name = it->second;
											}
											else
											{
												std::string localized = loc.localize(raw_str);
												if (!localized.empty() && localized[0] != '$')
												{
													loc_cache[raw_str] = localized;
													entry.display_name = localized;
												}
											}
										}
										else
										{
											entry.display_name = mono::from_mono_string(raw_name);
										}
									}
								}
								if (m_type_field)
								{
									int item_type = 0;
									mono::get_field_value(shared_obj, m_type_field, &item_type);
									// Map ItemType enum
									if (item_type == 2 || item_type == 3 || item_type == 9 || item_type == 15)
										entry.category = "Weapons";
									else if (item_type == 4 || item_type == 5 || item_type == 6 || item_type == 7 || item_type == 8 || item_type == 12 || item_type == 13)
										entry.category = "Armor & Gear";
									else if (item_type == 11)
										entry.category = "Food & Potions";
									else if (item_type == 1 || item_type == 16)
										entry.category = "Materials";
									else if (item_type == 10 || item_type == 14)
										entry.category = "Tools";
									else if (item_type == 18)
										entry.category = "Ammo";
									else
										entry.category = "Misc";
								}
								if (m_stack_field)
									mono::get_field_value(shared_obj, m_stack_field, &entry.max_stack);
								if (m_quality_field)
									mono::get_field_value(shared_obj, m_quality_field, &entry.max_quality);
							}
						}
					}
				}

				scanned.push_back(std::move(entry));
			}

			if (!scanned.empty())
			{
				const auto count = scanned.size();
				{
					std::lock_guard lock(g_items_mutex);
					g_items = std::move(scanned);
				}
				item_icons::invalidate();
				notification::success("Item Spawner", std::format("Loaded {} items from ObjectDB!", count));
			}
		}
		EXCEPT_CLAUSE

		s_is_refreshing = false;
	}

	void item_spawner::refresh_impl()
	{
		if (s_is_refreshing.exchange(true))
		{
			notification::warning("Item Spawner", "Database scan is already in progress!");
			return;
		}

		if (g_fiber_pool)
		{
			g_fiber_pool->queue_job([this] {
				scan_worker_impl();
			});
		}
		else
		{
			s_is_refreshing = false;
			notification::error("Item Spawner", "Fiber pool not available.");
		}
	}

	std::vector<item_spawner::item_entry> item_spawner::get_items_impl()
	{
		std::lock_guard lock(g_items_mutex);
		if (!g_initialized)
		{
			g_items = default_catalog;
			g_initialized = true;
		}
		return g_items;
	}

	std::vector<std::string> item_spawner::get_categories_impl()
	{
		return {"All", "Weapons", "Armor & Gear", "Food & Potions", "Materials", "Tools", "Ammo", "Misc"};
	}

	void item_spawner::spawn_to_inventory_worker_impl(const std::string& prefab_name, int amount, int quality)
	{
		TRY_CLAUSE
		{
			auto player_obj = unity::get_local_player();
			if (!player_obj)
			{
				notification::warning("Item Spawner", "Local player not found. Join a world first!");
				return;
			}

			player p(player_obj);
			auto inv = p.get_inventory();
			if (!inv.get_object())
			{
				notification::warning("Item Spawner", "Inventory not available.");
				return;
			}

			int clamped_amount = std::max(1, amount);
			int clamped_quality = std::max(1, quality);

			// 1. Prefer native Valheim Inventory::AddItem(string name, int stack, int quality, int variant, long crafterID, string crafterName, bool cheated, bool pickedUp)
			static auto add_item_8_args = mono::get_method("Inventory", "AddItem", 8, "assembly_valheim");
			if (add_item_8_args)
			{
				auto ms_name = mono::to_mono_string(prefab_name);
				auto ms_crafter = mono::to_mono_string("");
				int variant = 0;
				int64_t crafter_id = 0;
				bool cheated = false;
				bool picked_up = true;

				void* args[8] = {
				    ms_name,
				    &clamped_amount,
				    &clamped_quality,
				    &variant,
				    &crafter_id,
				    ms_crafter,
				    &cheated,
				    &picked_up};

				auto result = mono::invoke_method(add_item_8_args, inv.get_object(), args);
				if (result)
				{
					notification::success("Item Spawner", std::format("Added {}x {} (Q{}) to inventory!", clamped_amount, prefab_name, clamped_quality));
					return;
				}
			}

			// 2. Fallback to prefab-based AddItem(GameObject, int)
			auto obj_db = unity::get_object_db();
			MonoObject* prefab = nullptr;

			if (obj_db)
			{
				static auto get_prefab_method = mono::get_method("ObjectDB", "GetItemPrefab", 1, "assembly_valheim");
				if (get_prefab_method)
				{
					auto ms = mono::to_mono_string(prefab_name);
					void* args[1] = {ms};
					prefab = mono::invoke_method(get_prefab_method, obj_db, args);
				}
			}

			if (!prefab)
			{
				auto znet = unity::get_znet_scene();
				if (znet)
				{
					static auto znet_get_prefab = mono::get_method("ZNetScene", "GetPrefab", 1, "assembly_valheim");
					if (znet_get_prefab)
					{
						auto ms = mono::to_mono_string(prefab_name);
						void* args[1] = {ms};
						prefab = mono::invoke_method(znet_get_prefab, znet, args);
					}
				}
			}

			if (!prefab)
			{
				notification::warning("Item Spawner", std::format("Prefab '{}' not found in database.", prefab_name));
				return;
			}

			static auto add_item_method = mono::get_method_overload("Inventory", "AddItem", 2, "Boolean", "GameObject", "assembly_valheim");
			if (!add_item_method)
				add_item_method = mono::get_method("Inventory", "AddItem", 2, "assembly_valheim");

			if (!add_item_method)
			{
				notification::error("Item Spawner", "Inventory::AddItem method not resolved.");
				return;
			}

			void* add_args[2] = {prefab, &clamped_amount};
			auto ret = mono::invoke_method(add_item_method, inv.get_object(), add_args);
			if (!ret || !*static_cast<bool*>(mono::object_unbox(ret)))
			{
				notification::warning("Item Spawner", "Inventory is full or could not add item.");
				return;
			}

			// Only if clamped_quality > 1, update quality on the target added item only!
			if (clamped_quality > 1)
			{
				auto inv_class = mono::get_class("Inventory", "assembly_valheim");
				auto get_all_method = inv_class ? mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim") : nullptr;
				if (get_all_method)
				{
					auto all_items = mono::invoke_method(get_all_method, inv.get_object(), nullptr);
					auto vec_items = unity::list_to_vector(all_items);
					for (auto it = vec_items.rbegin(); it != vec_items.rend(); ++it)
					{
						if (!*it)
							continue;
						item_data idata(*it);
						auto shared_obj = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(*it);
						if (!shared_obj)
							continue;

						auto drop_prefab = mono::get_field_value<"ItemDrop/ItemData", "m_dropPrefab", MonoObject*>(*it);
						std::string drop_name = drop_prefab ? unity::get_name(drop_prefab) : "";
						if (drop_name == prefab_name || (drop_name.empty() && idata.get_quality() == 1))
						{
							int max_q = mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxQuality", int>(shared_obj);
							int target_q = (max_q > 1) ? std::min(clamped_quality, max_q) : 1;
							idata.set_quality(target_q);
							idata.set_durability(idata.get_max_durability());
							break;
						}
					}
				}
			}

			notification::success("Item Spawner", std::format("Added {}x {} (Q{}) to inventory!", clamped_amount, prefab_name, clamped_quality));
		}
		EXCEPT_CLAUSE
	}

	bool item_spawner::spawn_to_inventory_impl(const std::string& prefab_name, int amount, int quality)
	{
		if (script::get_current())
		{
			spawn_to_inventory_worker_impl(prefab_name, amount, quality);
			return true;
		}

		if (g_fiber_pool)
		{
			g_fiber_pool->queue_job([this, prefab_name, amount, quality] {
				spawn_to_inventory_worker_impl(prefab_name, amount, quality);
			});
			return true;
		}
		return false;
	}

	void item_spawner::spawn_in_world_worker_impl(const std::string& prefab_name, int amount, int level)
	{
		TRY_CLAUSE
		{
			auto player_obj = unity::get_local_player();
			if (!player_obj)
			{
				notification::warning("Item Spawner", "Local player not found. Join a world first!");
				return;
			}

			auto znet = unity::get_znet_scene();
			if (!znet)
			{
				notification::warning("Item Spawner", "ZNetScene not available.");
				return;
			}

			static auto znet_get_prefab = mono::get_method("ZNetScene", "GetPrefab", 1, "assembly_valheim");
			if (!znet_get_prefab)
				return;

			auto ms = mono::to_mono_string(prefab_name);
			void* args[1] = {ms};
			MonoObject* prefab = mono::invoke_method(znet_get_prefab, znet, args);
			if (!prefab)
			{
				notification::warning("Item Spawner", std::format("Prefab '{}' not found in ZNetScene.", prefab_name));
				return;
			}

			Vector3 my_pos = unity::get_position(player_obj);
			Vector3 forward = unity::get_forward(player_obj);
			if (forward.length() < 0.1f)
				forward = Vector3{0.f, 0.f, 1.f};

			Vector3 spawn_pos = my_pos + forward * 2.5f + Vector3{0.f, 0.5f, 0.f};
			Vector4 spawn_rot{0.f, 0.f, 0.f, 1.f};

			static auto instantiate_method = mono::get_method("Object", "Instantiate", 3, "UnityEngine.CoreModule", "UnityEngine");
			if (!instantiate_method)
				return;

			int spawn_count = std::clamp(amount, 1, 20);
			for (int i = 0; i < spawn_count; ++i)
			{
				Vector3 offset_pos = spawn_pos + Vector3{(i % 5) * 0.8f, 0.f, (i / 5) * 0.8f};
				void* inst_args[3] = {prefab, &offset_pos, &spawn_rot};
				auto spawned = mono::invoke_method(instantiate_method, nullptr, inst_args);
				if (spawned && level > 1)
				{
					character ch(spawned);
					if (ch)
						ch.set_level(level);
				}
			}

			notification::success("World Spawner", std::format("Spawned {}x {} at your position!", spawn_count, prefab_name));
		}
		EXCEPT_CLAUSE
	}

	bool item_spawner::spawn_in_world_impl(const std::string& prefab_name, int amount, int level)
	{
		if (script::get_current())
		{
			spawn_in_world_worker_impl(prefab_name, amount, level);
			return true;
		}

		if (g_fiber_pool)
		{
			g_fiber_pool->queue_job([this, prefab_name, amount, level] {
				spawn_in_world_worker_impl(prefab_name, amount, level);
			});
			return true;
		}
		return false;
	}


	void item_spawner::toggle_standalone_window_impl()
	{
		s_standalone_open = !s_standalone_open;
	}

	bool item_spawner::is_standalone_window_open_impl()
	{
		return s_standalone_open;
	}

	void item_spawner::draw_standalone_window_impl()
	{
		if (!s_standalone_open)
			return;

		auto* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowSize({500.f, 520.f}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos({viewport->WorkPos.x + 40.f, viewport->WorkPos.y + 40.f}, ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Valheim Item & Entity Spawner", &s_standalone_open))
		{
			draw_menu_ui();
		}
		ImGui::End();
	}

	void item_spawner::draw_menu_ui_impl()
	{
		initialize();

		ImGui::TextUnformatted("Search item or prefab name:");
		ImGui::SetNextItemWidth(-1.f);
		ImGui::InputTextWithHint("##ItemSearch", "Search by name (e.g. Mistwalker, Iron, Shield)...", s_search_buffer, sizeof(s_search_buffer));

		ImGui::Spacing();

		auto categories = get_categories();
		if (s_selected_category < 0 || s_selected_category >= (int)categories.size())
			s_selected_category = 0;

		if (ImGui::BeginCombo("Category Filter", categories[s_selected_category].c_str()))
		{
			for (int i = 0; i < (int)categories.size(); ++i)
			{
				bool is_sel = (s_selected_category == i);
				if (ImGui::Selectable(categories[i].c_str(), is_sel))
					s_selected_category = i;
			}
			ImGui::EndCombo();
		}

		if (is_refreshing())
		{
			ImGui::BeginDisabled();
			ImGui::Button("Scanning ObjectDB... Please wait##RescanBtn", ImVec2(-1.f, 0.f));
			ImGui::EndDisabled();
		}
		else
		{
			if (ImGui::Button("Rescan Game Database (ObjectDB)", ImVec2(-1.f, 0.f)))
			{
				refresh();
			}
		}

		ImGui::Separator();

		const auto& all_items = get_items();
		std::string filter_str = s_search_buffer;
		std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), ::tolower);

		std::vector<const item_entry*> filtered;
		for (const auto& item : all_items)
		{
			if (s_selected_category > 0 && item.category != categories[s_selected_category])
				continue;

			if (!filter_str.empty())
			{
				std::string name_lower = item.display_name;
				std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
				std::string prefab_lower = item.prefab_name;
				std::transform(prefab_lower.begin(), prefab_lower.end(), prefab_lower.begin(), ::tolower);

				if (name_lower.find(filter_str) == std::string::npos && prefab_lower.find(filter_str) == std::string::npos)
					continue;
			}
			filtered.push_back(&item);
		}

		ImGui::Text("Matches: %zu items", filtered.size());

		ImGui::BeginChild("ItemListRegion", ImVec2(0, 180), true);
		constexpr float icon_size = 32.f;
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(filtered.size()), icon_size + ImGui::GetStyle().ItemSpacing.y);
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
			{
				const auto* item = filtered[i];
				ImGui::PushID(item->prefab_name.c_str());
				if (auto icon = item_icons::get(item->prefab_name))
					ImGui::Image(icon, ImVec2(icon_size, icon_size));
				else
					ImGui::Dummy(ImVec2(icon_size, icon_size));
				ImGui::SameLine();
				std::string label = std::format("{} [{}] ({})", item->display_name, item->prefab_name, item->category);
				bool is_selected = (s_selected_item_index == i);
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));
				if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(0, icon_size)))
				{
					s_selected_item_index = i;
					s_spawn_amount = std::min(s_spawn_amount, std::max(1, item->max_stack));
				}
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", label.c_str());
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		if (s_selected_item_index >= 0 && s_selected_item_index < (int)filtered.size())
		{
			const auto* cur = filtered[s_selected_item_index];
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.f, 1.f), "Selected: %s (%s)", cur->display_name.c_str(), cur->prefab_name.c_str());

			ImGui::SetNextItemWidth(180.f);
			ImGui::SliderInt("Amount / Stack", &s_spawn_amount, 1, 100);

			if (cur->max_quality > 1)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(140.f);
				ImGui::SliderInt("Quality Level", &s_spawn_quality, 1, 4);
			}

			ImGui::Spacing();
			if (ImGui::Button("Give to Inventory", ImVec2(180, 32)))
			{
				spawn_to_inventory(cur->prefab_name, s_spawn_amount, s_spawn_quality);
			}
			ImGui::SameLine();
			if (ImGui::Button("Spawn in World", ImVec2(160, 32)))
			{
				spawn_in_world(cur->prefab_name, s_spawn_amount, s_spawn_quality);
			}
		}
	}
}
