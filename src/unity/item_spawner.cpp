#include "item_spawner.hpp"
#include "item_icons.hpp"
#include "unity/item_data.hpp"
#include "unity/item_drop.hpp"
#include "unity/localization.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include "notification/notification_service.hpp"
#include "astra/host/canvas.hpp"
#include "menu_settings.hpp"
#include "task_mgr.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <cctype>
#include <format>
#include <imgui.h>
#include <unordered_map>
#include <unordered_set>

namespace big
{
	namespace
	{
		Vector3 get_crosshair_spawn_position()
		{
			const Vector3 fallback = unity::get_position(unity::get_local_player()) + unity::get_camera_forward() * 5.f + Vector3{0.f, 0.25f, 0.f};
			static auto get_main = mono::get_method("Camera", "get_main", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto get_transform = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");
			static auto raycast = mono::get_method_exact("Physics", "Raycast", {"UnityEngine.Vector3", "UnityEngine.Vector3", "UnityEngine.RaycastHit&", "System.Single"}, "UnityEngine.PhysicsModule", "UnityEngine");
			static auto hit_class       = mono::get_class("RaycastHit", "UnityEngine.PhysicsModule", "UnityEngine");
			static auto hit_point_field = hit_class ? mono::get_field(hit_class, "m_Point") : nullptr;
			if (!get_main || !get_transform || !get_position || !raycast || !hit_class || !hit_point_field)
			{
				return fallback;
			}

			auto camera     = mono::invoke_method(get_main, nullptr, nullptr);
			auto transform  = camera ? mono::invoke_method(get_transform, camera, nullptr) : nullptr;
			auto origin_obj = transform ? mono::invoke_method(get_position, transform, nullptr) : nullptr;
			if (!origin_obj)
			{
				return fallback;
			}

			Vector3 origin    = *reinterpret_cast<Vector3*>(mono::object_unbox(origin_obj));
			Vector3 direction = unity::get_camera_forward().normalize();
			if (direction.length() < 0.1f)
			{
				return fallback;
			}
			origin = origin + direction * 0.3f;

			auto hit_object = mono::object_new(hit_class);
			if (!hit_object)
			{
				return fallback;
			}
			float max_distance = 1000.f;
			void* args[4]      = {&origin, &direction, mono::object_unbox(hit_object), &max_distance};
			auto did_hit       = mono::invoke_method(raycast, nullptr, args);
			if (!did_hit || !*reinterpret_cast<bool*>(mono::object_unbox(did_hit)))
			{
				return fallback;
			}

			Vector3 point{};
			mono::get_field_value(hit_object, hit_point_field, &point);
			return point + Vector3{0.f, 0.25f, 0.f};
		}

		const char* get_item_category(int item_type)
		{
			switch (item_type)
			{
			case 1: // Material
				return "Materials";
			case 2:  // Consumable
			case 21: // Fish
				return "Food & Potions";
			case 3:  // OneHandedWeapon
			case 4:  // Bow
			case 14: // TwoHandedWeapon
			case 22: // TwoHandedWeaponLeft
				return "Weapons";
			case 5:  // Shield
			case 6:  // Helmet
			case 7:  // Chest
			case 11: // Legs
			case 12: // Hands
			case 17: // Shoulder
			case 18: // Utility
			case 24: // Trinket
				return "Armor & Gear";
			case 9:  // Ammo
			case 23: // AmmoNonEquipable
				return "Ammo";
			case 15: // Torch
			case 19: // Tool
				return "Tools";
			default: return "Misc";
			}
		}

		int get_runtime_variant_count(const std::string& prefab_name)
		{
			auto object_db = unity::get_object_db();
			static auto get_item_prefab = mono::get_method_overload("ObjectDB", "GetItemPrefab", 1, nullptr, "System.String", "assembly_valheim");
			static auto item_drop_class = mono::get_class("ItemDrop", "assembly_valheim");
			static auto get_component = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (!object_db || !get_item_prefab || !item_drop_class || !get_component)
			{
				return 1;
			}

			auto name           = mono::to_mono_string(prefab_name);
			void* prefab_args[] = {name};
			auto prefab         = mono::invoke_method(get_item_prefab, object_db, prefab_args);
			if (!prefab)
			{
				return 1;
			}

			auto item_type              = mono::reflection_type(item_drop_class);
			void* component_args[]      = {item_type};
			auto item_drop              = mono::invoke_method(get_component, prefab, component_args);
			static auto item_data_field = mono::get_field("ItemDrop", "m_itemData", "assembly_valheim");
			MonoObject* item_data       = nullptr;
			if (!item_drop || !item_data_field)
			{
				return 1;
			}
			mono::get_field_value(item_drop, item_data_field, &item_data);
			static auto shared_field = mono::get_field("ItemDrop/ItemData", "m_shared", "assembly_valheim");
			static auto variant_field = mono::get_field("ItemDrop/ItemData/SharedData", "m_variants", "assembly_valheim");
			MonoObject* shared = nullptr;
			if (!item_data || !shared_field || !variant_field)
			{
				return 1;
			}
			mono::get_field_value(item_data, shared_field, &shared);
			int count = 1;
			if (shared)
			{
				mono::get_field_value(shared, variant_field, &count);
			}
			return std::max(1, count);
		}

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
		std::call_once(s_history_load_once, [this] {
			s_favorites.insert(g_settings.self.item_spawner_favorites.begin(), g_settings.self.item_spawner_favorites.end());
			s_recent_items = g_settings.self.item_spawner_recent;
			s_recent_items.erase(std::remove(s_recent_items.begin(), s_recent_items.end(), std::string{}),
			    s_recent_items.end());
			std::vector<std::string> unique_recent;
			for (const auto& item : s_recent_items)
			{
				if (std::find(unique_recent.begin(), unique_recent.end(), item) == unique_recent.end())
				{
					unique_recent.push_back(item);
				}
			}
			s_recent_items = std::move(unique_recent);
			if (s_recent_items.size() > 20)
			{
				s_recent_items.resize(20);
			}
		});

		{
			std::lock_guard lock(g_items_mutex);
			if (!g_initialized || g_items.empty())
			{
				g_items       = default_catalog;
				g_initialized = true;
			}
		}

		if (unity::get_object_db() && !s_auto_scan_started.exchange(true))
		{
			refresh_impl();
		}
	}

	void item_spawner::persist_history_impl()
	{
		g_settings.self.item_spawner_favorites.assign(s_favorites.begin(), s_favorites.end());
		std::sort(g_settings.self.item_spawner_favorites.begin(), g_settings.self.item_spawner_favorites.end());
		g_settings.self.item_spawner_recent = s_recent_items;
		g_settings.attempt_save();
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
			auto item_drop_class   = mono::get_class("ItemDrop", "assembly_valheim");
			auto m_item_data_field = item_drop_class ? mono::get_field(item_drop_class, "m_itemData") : nullptr;
			auto item_data_class   = mono::get_class("ItemDrop/ItemData", "assembly_valheim");
			auto m_shared_field    = item_data_class ? mono::get_field(item_data_class, "m_shared") : nullptr;
			auto shared_class      = mono::get_class("ItemDrop/ItemData/SharedData", "assembly_valheim");
			auto m_name_field      = shared_class ? mono::get_field(shared_class, "m_name") : nullptr;
			auto m_type_field      = shared_class ? mono::get_field(shared_class, "m_itemType") : nullptr;
			auto m_stack_field     = shared_class ? mono::get_field(shared_class, "m_maxStackSize") : nullptr;
			auto m_quality_field   = shared_class ? mono::get_field(shared_class, "m_maxQuality") : nullptr;
			auto m_variants_field  = shared_class ? mono::get_field(shared_class, "m_variants") : nullptr;

			static auto comp_method = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (!comp_method)
			{
				comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			}

			auto mono_type = item_drop_class ? mono::reflection_type(item_drop_class) : nullptr;
			if (!item_drop_class || !m_item_data_field || !item_data_class || !m_shared_field || !m_name_field || !m_type_field || !m_stack_field || !m_quality_field || !m_variants_field || !comp_method || !mono_type)
			{
				notification::error("Item Spawner", "Required ObjectDB item metadata is unavailable; no partial catalog was loaded.");
				s_is_refreshing = false;
				return;
			}

			std::vector<item_spawner::item_entry> scanned;
			scanned.reserve(item_prefabs.size());
			std::unordered_set<std::string> seen;
			std::unordered_map<std::string, std::string> loc_cache;

			auto loc = localization::get_instance();

			for (auto* prefab : item_prefabs)
			{
				if (!prefab)
				{
					continue;
				}

				std::string p_name = unity::get_name(prefab);
				if (p_name.empty() || p_name == "unknown" || seen.contains(p_name))
				{
					continue;
				}

				void* args[1]  = {mono_type};
				auto drop_comp = mono::invoke_method(comp_method, prefab, args);
				if (!drop_comp)
				{
					continue;
				}
				MonoObject* item_data_obj = nullptr;
				mono::get_field_value(drop_comp, m_item_data_field, &item_data_obj);
				if (!item_data_obj)
				{
					continue;
				}
				MonoObject* shared_obj = nullptr;
				mono::get_field_value(item_data_obj, m_shared_field, &shared_obj);
				if (!shared_obj)
				{
					continue;
				}

				item_spawner::item_entry entry;
				entry.prefab_name   = p_name;
				entry.display_name  = p_name;
				entry.category      = "Misc";
				entry.max_stack     = 1;
				entry.max_quality   = 1;
				entry.variant_count = 1;

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
				int item_type = 0;
				mono::get_field_value(shared_obj, m_type_field, &item_type);
				entry.category = get_item_category(item_type);
				mono::get_field_value(shared_obj, m_stack_field, &entry.max_stack);
				mono::get_field_value(shared_obj, m_quality_field, &entry.max_quality);
				mono::get_field_value(shared_obj, m_variants_field, &entry.variant_count);
				entry.max_stack     = std::max(1, entry.max_stack);
				entry.max_quality   = std::max(1, entry.max_quality);
				entry.variant_count = std::max(1, entry.variant_count);
				seen.insert(p_name);

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

		async_run([this] {
			scan_worker_impl();
		});
	}

	std::vector<item_spawner::item_entry> item_spawner::get_items_impl()
	{
		std::lock_guard lock(g_items_mutex);
		if (!g_initialized)
		{
			g_items       = default_catalog;
			g_initialized = true;
		}
		return g_items;
	}

	std::vector<std::string> item_spawner::get_categories_impl()
	{
		return {"All", "Weapons", "Armor & Gear", "Food & Potions", "Materials", "Tools", "Ammo", "Misc"};
	}

	void item_spawner::spawn_to_inventory_worker_impl(const std::string& prefab_name, int amount, int quality, int variant, float durability, bool auto_equip)
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

			int clamped_amount          = std::clamp(amount, 1, 10000);
			int clamped_quality         = std::max(1, quality);
			int clamped_variant         = std::clamp(variant, 0, get_runtime_variant_count(prefab_name) - 1);
			float durability_ratio      = std::clamp(durability, 0.f, 1.f);
			int64_t crafter_id          = p.get_player_id();
			std::string crafter_name    = p.get_player_name();
			auto ms_crafter             = mono::to_mono_string(crafter_name);
			static auto inventory_class = mono::get_class("Inventory", "assembly_valheim");
			static auto get_all_method = inventory_class ? mono::get_method("Inventory", "GetAllItems", 0, "assembly_valheim") : nullptr;
			auto grid_key = [](MonoObject* item) {
				const auto position = item_data(item).get_grid_pos();
				return (static_cast<uint64_t>(static_cast<uint32_t>(position.x)) << 32) | static_cast<uint32_t>(position.y);
			};
			static auto inventory_changed = []() {
				auto m = mono::get_method("Inventory", "Changed", 2, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method("Inventory", "Changed", 0, "assembly_valheim");
				}
				return m;
			}();
			auto isolate_incompatible_stacks = [&](int incoming_quality) {
				std::vector<std::pair<MonoObject*, int>> isolated;
				if (!get_all_method)
				{
					return isolated;
				}

				static auto prefab_field = mono::get_field("ItemDrop/ItemData", "m_dropPrefab", "assembly_valheim");
				static auto world_level_field = mono::get_field("ItemDrop/ItemData", "m_worldLevel", "assembly_valheim");
				static auto custom_data_field = mono::get_field("ItemDrop/ItemData", "m_customData", "assembly_valheim");
				auto all_items  = mono::invoke_method(get_all_method, inv.get_object(), nullptr);
				int world_level = 0;
				if (auto world_level_address = mono::get_static_field_value("Game", "m_worldLevel", "assembly_valheim"))
				{
					world_level = *static_cast<int*>(world_level_address);
				}

				for (auto* item : unity::list_to_vector(all_items))
				{
					if (!item || item_data(item).get_quality() != incoming_quality)
					{
						continue;
					}

					MonoObject* item_prefab = nullptr;
					if (prefab_field)
					{
						mono::get_field_value(item, prefab_field, &item_prefab);
					}
					if (!item_prefab || unity::get_name(item_prefab) != prefab_name)
					{
						continue;
					}

					uint8_t item_world_level = 0;
					if (world_level_field)
					{
						mono::get_field_value(item, world_level_field, &item_world_level);
					}
					if (item_world_level != static_cast<uint8_t>(std::clamp(world_level, 0, 255)))
					{
						continue;
					}

					bool has_custom_data    = true;
					MonoObject* custom_data = nullptr;
					if (custom_data_field)
					{
						mono::get_field_value(item, custom_data_field, &custom_data);
						if (custom_data)
						{
							auto data_class = mono::object_get_class(custom_data);
							auto count_method = data_class ? mono::class_get_method_from_name(data_class, "get_Count", 0) : nullptr;
							auto count_object = count_method ? mono::invoke_method(count_method, custom_data, nullptr) : nullptr;
							if (count_object)
							{
								has_custom_data = *static_cast<int*>(mono::object_unbox(count_object)) > 0;
							}
						}
					}

					const bool metadata_matches = incoming_quality == clamped_quality && item_data(item).get_quality() == clamped_quality
					    && item_data(item).get_variant() == clamped_variant && !item_data(item).get_cheated()
					    && item_data(item).get_crafter_id() == crafter_id && item_data(item).get_crafter_name() == crafter_name && !has_custom_data
					    && std::abs(item_data(item).get_durability() - item_data(item).get_max_durability() * durability_ratio) < 0.01f;
					if (metadata_matches)
					{
						continue;
					}

					const int original_quality = item_data(item).get_quality();
					if (mono::set_field_value<"ItemDrop/ItemData", "m_quality">(item, 0))
					{
						isolated.emplace_back(item, original_quality);
					}
				}

				return isolated;
			};
			auto restore_isolated_stacks = [&](const std::vector<std::pair<MonoObject*, int>>& isolated) {
				for (const auto& [item, quality] : isolated)
				{
					mono::set_field_value<"ItemDrop/ItemData", "m_quality">(item, quality);
				}
				if (!isolated.empty() && inventory_changed)
				{
					if (mono::get_param_count(inventory_changed) == 2)
					{
						bool success  = true;
						bool cheated  = false;
						void* args[2] = {&success, &cheated};
						mono::invoke_method(inventory_changed, inv.get_object(), args);
					}
					else
					{
						mono::invoke_method(inventory_changed, inv.get_object(), nullptr);
					}
				}
			};

			auto get_stack_snapshot = [&]() {
				std::unordered_map<uint64_t, int> stacks;
				if (!get_all_method)
				{
					return stacks;
				}
				auto all_items = mono::invoke_method(get_all_method, inv.get_object(), nullptr);
				for (auto* item : unity::list_to_vector(all_items))
				{
					if (item)
					{
						stacks.emplace(grid_key(item), mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item));
					}
				}
				return stacks;
			};

			auto apply_item_options = [&](MonoObject* item) {
				if (!item)
				{
					return;
				}
				item_data data(item);
				const auto shared_obj = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item);
				const int max_quality = shared_obj ? mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxQuality", int>(shared_obj) : 1;
				data.set_quality(max_quality > 1 ? clamped_quality : 1);
				data.set_variant(clamped_variant);
				data.set_durability(data.get_max_durability() * durability_ratio);
				data.set_cheated(false);
				data.set_crafter_id(crafter_id);
				data.set_crafter_name(crafter_name);
			};

			auto configure_added_items = [&](const std::unordered_map<uint64_t, int>& before, MonoObject* returned_item) {
				std::vector<MonoObject*> changed_items;
				int added_units = 0;
				if (get_all_method)
				{
					auto all_items = mono::invoke_method(get_all_method, inv.get_object(), nullptr);
					for (auto* item : unity::list_to_vector(all_items))
					{
						if (!item)
						{
							continue;
						}
						const int stack = mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(item);
						auto previous   = before.find(grid_key(item));
						const int delta = stack - (previous == before.end() ? 0 : previous->second);
						if (delta > 0)
						{
							added_units += delta;
							changed_items.push_back(item);
						}
					}
				}

				if (changed_items.empty() && returned_item)
				{
					changed_items.push_back(returned_item);
					added_units = clamped_amount;
				}
				for (auto* item : changed_items)
				{
					apply_item_options(item);
				}
				return std::pair{std::move(changed_items), added_units};
			};

			// Prefer Valheim's native AddItem(string, int, int, int, long, string, bool, bool) or 7-param overload.
			static auto add_item_by_name = []() {
				auto m = mono::get_method_exact("Inventory", "AddItem", {"System.String", "System.Int32", "System.Int32", "System.Int32", "System.Int64", "System.String", "System.Boolean", "System.Boolean"}, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method_exact("Inventory", "AddItem", {"System.String", "System.Int32", "System.Int32", "System.Int32", "System.Int64", "System.String", "System.Boolean"}, "assembly_valheim");
				}
				return m;
			}();
			if (add_item_by_name)
			{
				auto before    = get_stack_snapshot();
				auto isolated  = isolate_incompatible_stacks(clamped_quality);
				auto ms_name   = mono::to_mono_string(prefab_name);
				bool picked_up = true;
				bool cheated   = false;

				MonoObject* result = nullptr;
				if (mono::get_param_count(add_item_by_name) == 8)
				{
					void* args[8] = {ms_name, &clamped_amount, &clamped_quality, &clamped_variant, &crafter_id, ms_crafter, &cheated, &picked_up};
					result = mono::invoke_method(add_item_by_name, inv.get_object(), args);
				}
				else
				{
					void* args[7] = {ms_name, &clamped_amount, &clamped_quality, &clamped_variant, &crafter_id, ms_crafter, &picked_up};
					result = mono::invoke_method(add_item_by_name, inv.get_object(), args);
				}
				restore_isolated_stacks(isolated);
				auto [changed_items, added_units] = configure_added_items(before, result);
				if (auto_equip && !changed_items.empty())
				{
					static auto equip_method = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
					if (equip_method)
					{
						bool trigger_effects = true;
						void* equip_args[2]  = {changed_items.back(), &trigger_effects};
						mono::invoke_method(equip_method, player_obj, equip_args);
					}
				}
				if (added_units >= clamped_amount)
				{
					notification::success("Item Spawner", std::format("Added {}x {} (Q{}) to inventory!", added_units, prefab_name, clamped_quality));
				}
				else if (added_units > 0)
				{
					notification::warning("Item Spawner", std::format("Added {} of {} requested {} (inventory capacity reached).", added_units, clamped_amount, prefab_name));
				}
				else
				{
					notification::warning("Item Spawner", "Inventory is full or the item could not be added.");
				}
				return;
			}

			// 2. Fallback to prefab-based AddItem(GameObject, int)
			auto obj_db        = unity::get_object_db();
			MonoObject* prefab = nullptr;

			if (obj_db)
			{
				static auto get_prefab_method = mono::get_method("ObjectDB", "GetItemPrefab", 1, "assembly_valheim");
				if (get_prefab_method)
				{
					auto ms       = mono::to_mono_string(prefab_name);
					void* args[1] = {ms};
					prefab        = mono::invoke_method(get_prefab_method, obj_db, args);
				}
			}

			if (!prefab)
			{
				prefab = unity::get_znet_prefab(prefab_name);
			}

			if (!prefab)
			{
				notification::warning("Item Spawner", std::format("Prefab '{}' not found in database.", prefab_name));
				return;
			}

			static auto add_item_method = mono::get_method_overload("Inventory", "AddItem", 2, "Boolean", "GameObject", "assembly_valheim");
			if (!add_item_method)
			{
				add_item_method = mono::get_method("Inventory", "AddItem", 2, "assembly_valheim");
			}

			if (!add_item_method)
			{
				notification::error("Item Spawner", "Inventory::AddItem method not resolved.");
				return;
			}

			auto drop_class         = mono::get_class("ItemDrop", "assembly_valheim");
			auto drop_field         = drop_class ? mono::get_field(drop_class, "m_itemData") : nullptr;
			MonoObject* prefab_data = nullptr;
			if (drop_field)
			{
				mono::get_field_value(prefab, drop_field, &prefab_data);
			}
			auto before = get_stack_snapshot();
			const int prefab_quality = prefab_data ? mono::get_field_value<"ItemDrop/ItemData", "m_quality", int>(prefab_data) : 1;
			auto isolated = isolate_incompatible_stacks(prefab_quality);
			auto shared_data = prefab_data ? mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(prefab_data) : nullptr;
			const int max_stack = shared_data ? std::max(1, mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxStackSize", int>(shared_data)) : 1;
			int remaining = clamped_amount;
			while (remaining > 0)
			{
				int stack_size    = std::min(remaining, max_stack);
				void* add_args[2] = {prefab, &stack_size};
				auto ret          = mono::invoke_method(add_item_method, inv.get_object(), add_args);
				if (!ret || !*static_cast<bool*>(mono::object_unbox(ret)))
				{
					break;
				}
				remaining -= stack_size;
			}
			restore_isolated_stacks(isolated);
			auto [changed_items, added_units] = configure_added_items(before, nullptr);
			if (auto_equip && !changed_items.empty())
			{
				static auto equip_method = mono::get_method("Humanoid", "EquipItem", 2, "assembly_valheim");
				if (equip_method)
				{
					bool trigger_effects = true;
					void* equip_args[2]  = {changed_items.back(), &trigger_effects};
					mono::invoke_method(equip_method, player_obj, equip_args);
				}
			}

			if (added_units >= clamped_amount)
			{
				notification::success("Item Spawner", std::format("Added {}x {} (Q{}) to inventory!", added_units, prefab_name, clamped_quality));
			}
			else if (added_units > 0)
			{
				notification::warning("Item Spawner", std::format("Added {} of {} requested {} (inventory capacity reached).", added_units, clamped_amount, prefab_name));
			}
			else
			{
				notification::warning("Item Spawner", "Inventory is full or could not add item.");
			}
		}
		EXCEPT_CLAUSE
	}

	bool item_spawner::spawn_to_inventory_impl(const std::string& prefab_name, int amount, int quality, int variant, float durability, bool auto_equip)
	{
		amount                  = std::clamp(amount, 1, 10000);
		quality                 = std::clamp(quality, 1, 99);
		s_last_spawned_prefab   = prefab_name;
		s_last_spawn_amount     = amount;
		s_last_spawn_quality    = quality;
		s_last_spawn_variant    = variant;
		s_last_spawn_durability = durability;
		s_last_spawn_auto_equip = auto_equip;
		s_last_spawn_to_world   = false;
		s_recent_items.erase(std::remove(s_recent_items.begin(), s_recent_items.end(), prefab_name), s_recent_items.end());
		s_recent_items.insert(s_recent_items.begin(), prefab_name);
		if (s_recent_items.size() > 20)
		{
			s_recent_items.resize(20);
		}
		persist_history_impl();
		async_run([this, prefab_name, amount, quality, variant, durability, auto_equip] {
			spawn_to_inventory_worker_impl(prefab_name, amount, quality, variant, durability, auto_equip);
		});
		return true;
	}

	void item_spawner::spawn_in_world_worker_impl(const std::string& prefab_name, int amount, int level, int variant, float durability, int placement)
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
			if (!unity::get_znet_scene())
			{
				LOG(WARNING) << std::format("Item Spawner: ZNetScene null (ObjectDB: {}, ZoneSystem: {})", unity::get_object_db() != nullptr, unity::get_zone_system() != nullptr);
				notification::warning("Item Spawner", "ZNetScene not available.");
				return;
			}

			MonoObject* prefab = unity::get_znet_prefab(prefab_name);
			if (!prefab)
			{
				notification::warning("Item Spawner", std::format("Prefab '{}' not found in ZNetScene.", prefab_name));
				return;
			}

			Vector3 my_pos  = unity::get_position(player_obj);
			Vector3 forward = unity::get_forward(player_obj);
			if (forward.length() < 0.1f)
			{
				forward = Vector3{0.f, 0.f, 1.f};
			}

			Vector3 spawn_pos = my_pos + forward * 2.5f + Vector3{0.f, 0.5f, 0.f};
			if (placement == 1)
			{
				spawn_pos = my_pos + Vector3{0.f, 0.25f, 0.f};
			}
			else if (placement == 2)
			{
				spawn_pos = get_crosshair_spawn_position();
			}
			Vector4 spawn_rot{0.f, 0.f, 0.f, 1.f};

			static auto instantiate_method = mono::get_method("Object", "Instantiate", 3, "UnityEngine.CoreModule", "UnityEngine");

			static auto comp_method = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			if (!comp_method)
			{
				comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			}

			auto item_drop_class     = mono::get_class("ItemDrop", "assembly_valheim");
			auto mono_item_drop_type = item_drop_class ? mono::reflection_type(item_drop_class) : nullptr;

			int64_t crafter_id       = p.get_player_id();
			std::string crafter_name = p.get_player_name();

			bool is_item = false;
			if (comp_method && mono_item_drop_type)
			{
				void* check_args[1] = {mono_item_drop_type};
				is_item             = (mono::invoke_method(comp_method, prefab, check_args) != nullptr);
			}

			if (is_item)
			{
				int max_stack        = 1;
				void* prefab_args[1] = {mono_item_drop_type};
				auto prefab_drop     = mono::invoke_method(comp_method, prefab, prefab_args);
				auto prefab_data = prefab_drop ? mono::get_field_value<"ItemDrop", "m_itemData", MonoObject*>(prefab_drop) : nullptr;
				auto clone_method            = mono::get_method("ItemDrop/ItemData", "Clone", 0, "assembly_valheim");
				static auto drop_item_method = mono::get_method("ItemDrop", "DropItem", 4, "assembly_valheim");
				if (prefab_drop)
				{
					auto shared_obj = prefab_data ? mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(prefab_data) : nullptr;
					if (shared_obj)
					{
						max_stack = std::max(1, mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxStackSize", int>(shared_obj));
					}
				}
				if (!prefab_data || !clone_method || !drop_item_method)
				{
					notification::error("World Spawner", "Valheim's native item drop method is unavailable.");
					return;
				}

				int remaining             = std::clamp(amount, 1, 10000);
				const int requested_units = remaining;
				int spawned_units         = 0;
				int stack_index           = 0;
				static auto world_level_address = mono::get_static_field_value("Game", "m_worldLevel", "assembly_valheim");
				const int game_world_level = world_level_address ? *static_cast<int*>(world_level_address) : 0;
				while (remaining > 0)
				{
					int stack_size     = std::min(remaining, max_stack);
					Vector3 offset_pos = spawn_pos + Vector3{(stack_index % 5) * 0.8f, 0.f, (stack_index / 5) * 0.8f};
					auto item_data_obj = mono::invoke_method(clone_method, prefab_data, nullptr);
					if (!item_data_obj)
					{
						break;
					}
					item_data idata(item_data_obj);
					mono::set_field_value<"ItemDrop/ItemData", "m_dropPrefab">(item_data_obj, prefab);
					auto shared_obj = mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(item_data_obj);
					int max_quality = shared_obj ? mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_maxQuality", int>(shared_obj) : 1;
					idata.set_quality(max_quality > 1 ? std::max(level, 1) : 1);
					int variant_count = shared_obj ? mono::get_field_value<"ItemDrop/ItemData/SharedData", "m_variants", int>(shared_obj) : 1;
					idata.set_variant(std::clamp(variant, 0, std::max(1, variant_count) - 1));
					idata.set_durability(idata.get_max_durability() * std::clamp(durability, 0.f, 1.f));
					idata.set_cheated(false);
					idata.set_crafter_id(crafter_id);
					idata.set_crafter_name(crafter_name);
					uint8_t world_level = static_cast<uint8_t>(std::clamp(game_world_level, 0, 255));
					mono::set_field_value<"ItemDrop/ItemData", "m_worldLevel">(item_data_obj, world_level);
					void* drop_args[4] = {item_data_obj, &stack_size, &offset_pos, &spawn_rot};
					auto spawned_drop  = mono::invoke_method(drop_item_method, nullptr, drop_args);
					if (!spawned_drop)
					{
						break;
					}
					spawned_units += stack_size;
					remaining -= stack_size;
					++stack_index;
				}
				if (spawned_units == requested_units)
				{
					notification::success("World Spawner", std::format("Spawned {}x {} (Q{}) in {} drop(s).", spawned_units, prefab_name, level, stack_index));
				}
				else
				{
					notification::warning("World Spawner", std::format("Spawned {} of {} requested {} units.", spawned_units, amount, prefab_name));
				}
			}
			else
			{
				if (!instantiate_method)
				{
					notification::error("World Spawner", "Unity prefab instantiate method is unavailable.");
					return;
				}
				int spawn_count = std::clamp(amount, 1, 20);
				for (int i = 0; i < spawn_count; ++i)
				{
					Vector3 offset_pos = spawn_pos + Vector3{(i % 5) * 0.8f, 0.f, (i / 5) * 0.8f};
					void* inst_args[3] = {prefab, &offset_pos, &spawn_rot};
					auto spawned       = mono::invoke_method(instantiate_method, nullptr, inst_args);
					if (spawned && level > 1)
					{
						character ch(spawned);
						if (ch)
						{
							ch.set_level(level);
						}
					}
				}
				notification::success("World Spawner", std::format("Spawned {}x {} at your position!", spawn_count, prefab_name));
			}
		}
		EXCEPT_CLAUSE
	}

	bool item_spawner::spawn_in_world_impl(const std::string& prefab_name, int amount, int level, int variant, float durability, int placement)
	{
		amount                  = std::clamp(amount, 1, 10000);
		level                   = std::clamp(level, 1, 99);
		s_last_spawned_prefab   = prefab_name;
		s_last_spawn_amount     = amount;
		s_last_spawn_quality    = level;
		s_last_spawn_variant    = variant;
		s_last_spawn_durability = durability;
		s_last_spawn_placement  = std::clamp(placement, 0, 2);
		s_last_spawn_auto_equip = false;
		s_last_spawn_to_world   = true;
		s_recent_items.erase(std::remove(s_recent_items.begin(), s_recent_items.end(), prefab_name), s_recent_items.end());
		s_recent_items.insert(s_recent_items.begin(), prefab_name);
		if (s_recent_items.size() > 20)
		{
			s_recent_items.resize(20);
		}
		persist_history_impl();
		async_run([this, prefab_name, amount, level, variant, durability, placement]() -> task<void> {
			// ZNetScene is recreated on every world load and stays null until its Awake runs.
			for (int i = 0; i < 30 && unity::get_local_player() && !unity::get_znet_scene(); ++i)
			{
				co_await this_task::wait(std::chrono::milliseconds(100));
			}
			spawn_in_world_worker_impl(prefab_name, amount, level, variant, durability, placement);
		});
		return true;
	}


	void item_spawner::toggle_standalone_window_impl()
	{
		s_standalone_open = !s_standalone_open;
	}

	bool item_spawner::is_standalone_window_open_impl()
	{
		return s_standalone_open;
	}

	void item_spawner::focus_item_impl(const std::string& prefab_name)
	{
		if (prefab_name.empty())
		{
			return;
		}

		initialize_impl();
		s_selected_prefab   = prefab_name;
		s_selected_category = 0;
		s_favorites_only    = false;
		s_recent_only       = false;
		std::snprintf(s_search_buffer, sizeof(s_search_buffer), "%s", prefab_name.c_str());
		s_standalone_open = true;

		const auto items = get_items_impl();
		if (std::none_of(items.begin(), items.end(), [&](const item_entry& item) {
			    return item.prefab_name == prefab_name;
		    }))
		{
			refresh_impl();
		}
	}

	void item_spawner::draw_standalone_window_impl()
	{
		if (!s_standalone_open)
		{
			return;
		}

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
		if (ImGui::Checkbox("Favorites only", &s_favorites_only))
		{
			s_recent_only = false;
		}
		ImGui::SameLine();
		if (ImGui::Checkbox("Recent only", &s_recent_only))
		{
			s_favorites_only = false;
		}

		auto categories = get_categories();
		if (s_selected_category < 0 || s_selected_category >= (int)categories.size())
		{
			s_selected_category = 0;
		}

		if (ImGui::BeginCombo("Category Filter", categories[s_selected_category].c_str()))
		{
			for (int i = 0; i < (int)categories.size(); ++i)
			{
				bool is_sel = (s_selected_category == i);
				if (ImGui::Selectable(categories[i].c_str(), is_sel))
				{
					s_selected_category = i;
				}
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

		const auto& all_items  = get_items();
		std::string filter_str = s_search_buffer;
		auto lowercase         = [](std::string& text) {
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char value) {
				return static_cast<char>(std::tolower(value));
			});
		};
		lowercase(filter_str);

		std::vector<const item_entry*> filtered;
		for (const auto& item : all_items)
		{
			if (s_favorites_only && !s_favorites.contains(item.prefab_name))
			{
				continue;
			}
			if (s_recent_only
			    && std::find(s_recent_items.begin(), s_recent_items.end(), item.prefab_name) == s_recent_items.end())
			{
				continue;
			}
			if (s_selected_category > 0 && item.category != categories[s_selected_category])
			{
				continue;
			}

			if (!filter_str.empty())
			{
				std::string name_lower = item.display_name;
				lowercase(name_lower);
				std::string prefab_lower = item.prefab_name;
				lowercase(prefab_lower);

				if (name_lower.find(filter_str) == std::string::npos && prefab_lower.find(filter_str) == std::string::npos)
				{
					continue;
				}
			}
			filtered.push_back(&item);
		}
		if (s_recent_only)
		{
			std::stable_sort(filtered.begin(), filtered.end(), [this](const item_entry* left, const item_entry* right) {
				auto left_pos  = std::find(s_recent_items.begin(), s_recent_items.end(), left->prefab_name);
				auto right_pos = std::find(s_recent_items.begin(), s_recent_items.end(), right->prefab_name);
				return left_pos < right_pos;
			});
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
				{
					ImGui::Image(icon, ImVec2(icon_size, icon_size));
				}
				else
				{
					ImGui::Dummy(ImVec2(icon_size, icon_size));
				}
				ImGui::SameLine();
				std::string label = std::format("{} [{}] ({})", item->display_name, item->prefab_name, item->category);
				bool is_selected  = (s_selected_prefab == item->prefab_name);
				ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.f, 0.5f));
				if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(0, icon_size)))
				{
					s_selected_item_index = i;
					s_selected_prefab     = item->prefab_name;
					s_spawn_amount        = std::min(s_spawn_amount, std::max(1, item->max_stack));
				}
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", label.c_str());
				}
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		const item_entry* selected = nullptr;
		for (const auto* item : filtered)
		{
			if (item->prefab_name == s_selected_prefab)
			{
				selected = item;
				break;
			}
		}
		if (!selected && !filtered.empty())
		{
			selected              = filtered.front();
			s_selected_prefab     = selected->prefab_name;
			s_selected_item_index = 0;
		}

		if (selected)
		{
			const auto* cur = selected;
			s_spawn_variant = std::clamp(s_spawn_variant, 0, std::max(1, cur->variant_count) - 1);
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.f, 1.f),
			    "Selected: %s (%s)",
			    cur->display_name.c_str(),
			    cur->prefab_name.c_str());
			if (ImGui::SmallButton(s_favorites.contains(cur->prefab_name) ? "Remove Favorite" : "Add Favorite"))
			{
				if (s_favorites.contains(cur->prefab_name))
				{
					s_favorites.erase(cur->prefab_name);
				}
				else
				{
					s_favorites.insert(cur->prefab_name);
				}
				persist_history_impl();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy Item ID"))
			{
				ImGui::SetClipboardText(cur->prefab_name.c_str());
				notification::success("Item Spawner", "Prefab ID copied to clipboard.");
			}

			ImGui::SetNextItemWidth(180.f);
			ImGui::SliderInt("Quantity", &s_spawn_amount, 1, 10000);
			if (ImGui::SmallButton("10"))
			{
				s_spawn_amount = 10;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("50"))
			{
				s_spawn_amount = 50;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("100"))
			{
				s_spawn_amount = 100;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Max Stack"))
			{
				s_spawn_amount = std::clamp(cur->max_stack, 1, 10000);
			}

			if (cur->max_quality > 1)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(140.f);
				ImGui::SliderInt("Quality Level", &s_spawn_quality, 1, 99);
			}
			if (cur->variant_count > 1)
			{
				ImGui::SetNextItemWidth(180.f);
				ImGui::SliderInt("Variant", &s_spawn_variant, 0, cur->variant_count - 1);
				ImGui::SameLine();
			}
			ImGui::SetNextItemWidth(180.f);
			int durability_percent = static_cast<int>(std::lround(s_spawn_durability * 100.f));
			if (ImGui::SliderInt("Durability %", &durability_percent, 0, 100))
			{
				s_spawn_durability = durability_percent / 100.f;
			}
			ImGui::Checkbox("Auto equip when possible", &s_auto_equip);
			const char* spawn_placements[] = {"In front", "At feet", "Along crosshair direction"};
			ImGui::Combo("World spawn position", &s_spawn_placement, spawn_placements, IM_ARRAYSIZE(spawn_placements));

			ImGui::Spacing();
			if (ImGui::Button("Give to Inventory", ImVec2(180, 32)))
			{
				spawn_to_inventory(cur->prefab_name, s_spawn_amount, s_spawn_quality, s_spawn_variant, s_spawn_durability, s_auto_equip);
			}
			ImGui::SameLine();
			if (ImGui::Button("Spawn in World", ImVec2(160, 32)))
			{
				spawn_in_world(cur->prefab_name, s_spawn_amount, s_spawn_quality, s_spawn_variant, s_spawn_durability, s_spawn_placement);
			}
			if (!s_last_spawned_prefab.empty() && ImGui::Button("Spawn Again", ImVec2(180, 28)))
			{
				if (s_last_spawn_to_world)
				{
					spawn_in_world(s_last_spawned_prefab, s_last_spawn_amount, s_last_spawn_quality, s_last_spawn_variant, s_last_spawn_durability, s_last_spawn_placement);
				}
				else
				{
					spawn_to_inventory(s_last_spawned_prefab, s_last_spawn_amount, s_last_spawn_quality, s_last_spawn_variant, s_last_spawn_durability, s_last_spawn_auto_equip);
				}
			}
		}
	}
}
