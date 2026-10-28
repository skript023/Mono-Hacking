#include "base_tools.hpp"
#include "esp_projection.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "unity/localization.hpp"
#include "utility/unity.hpp"
#include "pointers.hpp"
#include <astra/host/notification.hpp>

namespace big
{
	namespace
	{
		MonoClassField* find_field(MonoClass* klass, const char* name)
		{
			for (auto k = klass; k != nullptr; k = mono::class_get_parent(k))
			{
				if (auto f = mono::get_field(k, name))
				{
					return f;
				}
			}
			return nullptr;
		}
		MonoMethod* find_method(MonoClass* klass, const char* name, int param_count = 0)
		{
			for (auto k = klass; k != nullptr; k = mono::class_get_parent(k))
			{
				if (auto m = mono::class_get_method_from_name(k, name, param_count))
				{
					return m;
				}
			}
			return nullptr;
		}
		template<typename T>
		T field(MonoObject* object, const char* name)
		{
			T value{};
			if (object)
			{
				if (auto f = find_field(mono::object_get_class(object), name))
				{
					mono::get_field_value(object, f, &value);
				}
			}
			return value;
		}
		MonoObject* call(MonoObject* object, const char* name)
		{
			if (!object)
			{
				return nullptr;
			}
			auto method = find_method(mono::object_get_class(object), name, 0);
			return method ? mono::invoke_method(method, object) : nullptr;
		}
		template<typename T>
		T value(MonoObject* object, const char* name)
		{
			auto result = call(object, name);
			return result ? *static_cast<T*>(mono::object_unbox(result)) : T{};
		}
		bool valid(MonoObject* object)
		{
			auto view = field<MonoObject*>(object, "m_nview");
			return view && value<bool>(view, "IsValid");
		}
		bool take_ownership(MonoObject* object)
		{
			auto view = field<MonoObject*>(object, "m_nview");
			if (!view || !value<bool>(view, "IsValid"))
			{
				return false;
			}
			if (!value<bool>(view, "IsOwner"))
			{
				call(view, "ClaimOwnership");
			}
			return value<bool>(view, "IsOwner");
		}
		Vector3 position(MonoObject* object)
		{
			return unity::get_position(object);
		}
		float distance_squared(Vector3 a, Vector3 b)
		{
			auto d = a - b;
			return d.x * d.x + d.y * d.y + d.z * d.z;
		}
		std::vector<MonoObject*> scene_objects(const char* name)
		{
			static const std::unordered_map<std::string_view, const char*> instance_lists = {{"WearNTear", "s_allInstances"}, {"ShieldGenerator", "m_instances"}};
			if (auto list = instance_lists.find(name); list != instance_lists.end())
			{
				auto slot = static_cast<MonoObject**>(mono::get_static_field_value(name, list->second, "assembly_valheim"));
				return slot && *slot ? unity::list_to_vector(*slot) : std::vector<MonoObject*>{};
			}

			static auto find = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			auto type = mono::reflection_type(mono::get_class(name, "assembly_valheim"));
			if (!find || !type)
			{
				return {};
			}
			void* args[] = {type};
			auto array   = reinterpret_cast<MonoArray*>(mono::invoke_method(find, nullptr, args));
			if (!array)
			{
				return {};
			}
			std::vector<MonoObject*> objects;
			for (auto object : mono_array_view<MonoObject*>(array))
			{
				objects.push_back(object);
			}
			return objects;
		}
		std::vector<MonoObject*> nearby(const char* name, float radius)
		{
			std::vector<MonoObject*> result;
			auto local = unity::get_local_player();
			if (!local)
			{
				return result;
			}
			const auto origin = player(local).get_position();
			std::vector<std::pair<float, MonoObject*>> found;
			for (auto object : scene_objects(name))
			{
				if (!object)
				{
					continue;
				}
				const float distance = distance_squared(origin, position(object));
				if (distance <= radius * radius)
				{
					found.emplace_back(distance, object);
				}
			}
			std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
				return a.first < b.first;
			});
			result.reserve(found.size());
			for (const auto& entry : found)
			{
				result.push_back(entry.second);
			}
			return result;
		}
		std::string name(MonoObject* object)
		{
			if (!object)
			{
				return {};
			}
			auto token = mono::from_mono_string(field<MonoString*>(object, "m_name"));
			if (token.empty())
			{
				if (auto piece = field<MonoObject*>(object, "m_piece"))
				{
					token = mono::from_mono_string(field<MonoString*>(piece, "m_name"));
				}
			}
			if (token.empty())
			{
				token = mono::from_mono_string(reinterpret_cast<MonoString*>(call(object, "get_name")));
			}
			return localization::get_instance().localize(token);
		}
		bool access(Vector3 point)
		{
			static auto method = mono::get_method("PrivateArea", "CheckAccess", 4, "assembly_valheim");
			if (!method)
			{
				return true;
			}
			float radius = 0.f;
			bool flash = false, ward = true;
			void* args[] = {&point, &radius, &flash, &ward};
			auto result  = mono::invoke_method(method, nullptr, args);
			return result ? *static_cast<bool*>(mono::object_unbox(result)) : true;
		}
	}

	base_tools::options base_tools::get_options_impl()
	{
		std::lock_guard lock(options_mutex);
		return config;
	}
	void base_tools::set_options_impl(options v)
	{
		v.radius = std::clamp(v.radius, 5.f, 100.f);
		for (auto p : {&v.smelter_speed, &v.fermenter_speed, &v.honey_speed, &v.plant_speed, &v.cooking_speed, &v.sap_speed})
		{
			*p = std::clamp(*p, 1.f, 50.f);
		}
		v.daylight       = std::clamp(v.daylight, 0.f, 1.f);
		v.wind_direction = std::fmod(v.wind_direction + 3600.f, 360.f);
		v.wind_strength  = std::clamp(v.wind_strength, 0.f, 2.f);
		std::lock_guard lock(options_mutex);
		config = std::move(v);
	}
	base_tools::snapshot base_tools::get_snapshot_impl()
	{
		std::lock_guard lock(snapshot_mutex);
		return data;
	}
	void base_tools::request_scan_impl()
	{
		scan_requested = true;
	}
	std::vector<base_tools::marker> base_tools::get_markers_impl()
	{
		std::lock_guard lock(snapshot_mutex);
		return std::chrono::steady_clock::now() - projected < std::chrono::milliseconds(500) ? markers : std::vector<marker>{};
	}
	float base_tools::multiplier_impl(MonoObject* object, float speed)
	{
		if (speed <= 1.f || !object || !valid(object))
		{
			return 1.f;
		}
		auto local = unity::get_local_player();
		if (!local || !value<bool>(field<MonoObject*>(object, "m_nview"), "IsOwner"))
		{
			return 1.f;
		}
		const float radius = option_impl(&options::radius);
		return distance_squared(position(object), player(local).get_position()) <= radius * radius ? speed : 1.f;
	}

	void base_tools::quick_stack_impl()
	{
		const auto cfg = get_options();
		int requested  = 0;
		auto now       = std::chrono::steady_clock::now();
		for (auto it = pending_stacks.begin(); it != pending_stacks.end();)
		{
			if (now - it->second > std::chrono::seconds(5))
			{
				it = pending_stacks.erase(it);
			}
			else
			{
				++it;
			}
		}

		auto containers = nearby("Container", cfg.radius);
		for (auto chest : containers)
		{
			if (!valid(chest) || value<bool>(chest, "IsInUse") || !access(position(chest)))
			{
				continue;
			}
			const int id = value<int>(chest, "GetInstanceID");
			if (id != 0 && pending_stacks.contains(id))
			{
				continue;
			}
			if (id != 0)
			{
				pending_stacks[id] = now;
			}
			auto view = field<MonoObject*>(chest, "m_nview");
			if (view && !value<bool>(view, "IsOwner"))
			{
				call(view, "ClaimOwnership");
			}
			call(chest, "StackAll");
			++requested;
		}
		if (requested > 0)
		{
			notification::info("Quick Stack", std::format("Requested stacking into {} nearby chest(s).", requested));
		}
		else if (containers.empty())
		{
			notification::info("Quick Stack", "No chests found within range.");
		}
		else
		{
			notification::info("Quick Stack", "Chests found in range, but none were accessible or free.");
		}
	}
	bool base_tools::begin_stack_response_impl(MonoObject* chest)
	{
		const int id = value<int>(chest, "GetInstanceID");
		auto it      = pending_stacks.find(id);
		if (it == pending_stacks.end())
		{
			return false;
		}
		pending_stacks.erase(it);
		// Keep protection even if the server answers a request late.
		return true;
	}
	bool base_tools::protected_item_impl(MonoObject* item)
	{
		if (!item)
		{
			return true;
		}
		const auto cfg = get_options();
		auto shared    = field<MonoObject*>(item, "m_shared");
		int type       = field<int>(shared, "m_itemType");
		auto grid_pos  = field<iVector2>(item, "m_gridPos");
		return (cfg.keep_hotbar && grid_pos.y == 0) || (cfg.keep_food && type == 2) || (cfg.keep_ammo && (type == 9 || type == 23));
	}
	void base_tools::repair_nearby_impl()
	{
		int count = 0;
		for (auto piece : nearby("WearNTear", get_options().radius))
		{
			if (valid(piece) && access(position(piece)) && value<bool>(piece, "Repair"))
			{
				++count;
			}
		}
		notification::info("Repair Radius", std::format("Sent {} repair requests.", count));
		request_scan();
	}
	void base_tools::grow_nearby_impl()
	{
		int count = 0;
		for (auto plant : nearby("Plant", get_options().radius))
		{
			if (!valid(plant) || !access(position(plant)))
			{
				continue;
			}
			if (!take_ownership(plant))
			{
				continue;
			}
			auto status_field = find_field(mono::object_get_class(plant), "m_status");
			if (!status_field)
			{
				continue;
			}
			int healthy = 0;
			mono::set_field_value(plant, status_field, &healthy);
			if (call(plant, "Grow"))
			{
				++count;
			}
		}
		notification::success("Farming", std::format("Instantly matured {} plant(s) (including harsh biomes like Deep North).", count));
		request_scan();
	}
	void base_tools::instant_finish_nearby_impl()
	{
		const auto cfg = get_options();
		int count      = 0;
		for (auto object : nearby("Smelter", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			auto* smelter_class       = mono::object_get_class(object);
			auto get_queue_size       = find_method(smelter_class, "GetQueueSize");
			auto get_queued_ore       = find_method(smelter_class, "GetQueuedOre");
			auto remove_one_ore       = find_method(smelter_class, "RemoveOneOre");
			auto queue_processed      = find_method(smelter_class, "QueueProcessed", 1);
			auto spawn_processed      = find_method(smelter_class, "SpawnProcessed");
			auto processed_queue_size = find_method(smelter_class, "GetProcessedQueueSize");
			if (!get_queue_size || !get_queued_ore || !remove_one_ore || !queue_processed || !spawn_processed)
			{
				continue;
			}

			if (processed_queue_size)
			{
				count += std::max(0, value<int>(object, "GetProcessedQueueSize"));
			}
			const int queued = std::clamp(value<int>(object, "GetQueueSize"), 0, 10000);
			for (int i = 0; i < queued; ++i)
			{
				auto ore = mono::invoke_method(get_queued_ore, object, nullptr);
				if (!ore || mono::from_mono_string(reinterpret_cast<MonoString*>(ore)).empty())
				{
					break;
				}
				void* ore_args[]      = {ore};
				MonoObject* exception = nullptr;
				mono::invoke_method(queue_processed, object, ore_args, &exception);
				if (exception)
				{
					break;
				}
				mono::invoke_method(remove_one_ore, object, nullptr, &exception);
				if (exception)
				{
					break;
				}
				++count;
			}
			mono::invoke_method(spawn_processed, object, nullptr);
		}
		for (auto object : nearby("CookingStation", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}

			static auto slots_field = mono::get_field("CookingStation", "m_slots", "assembly_valheim");
			static auto get_slot    = []() {
				auto m = mono::get_method("CookingStation", "GetSlot", 5, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method("CookingStation", "GetSlot", 4, "assembly_valheim");
				}
				return m;
			}();
			static auto set_slot = []() {
				auto m = mono::get_method("CookingStation", "SetSlot", 5, "assembly_valheim");
				if (!m)
				{
					m = mono::get_method("CookingStation", "SetSlot", 4, "assembly_valheim");
				}
				return m;
			}();
			static auto get_conversion = mono::get_method("CookingStation", "GetItemConversion", 1, "assembly_valheim");
			static auto conversion_output = mono::get_field("CookingStation/ItemConversion", "m_to", "assembly_valheim");
			static auto conversion_time = mono::get_field("CookingStation/ItemConversion", "m_cookTime", "assembly_valheim");
			static auto update_visual = mono::get_method("CookingStation", "UpdateVisual", 1, "assembly_valheim");
			static auto is_fire_lit   = mono::get_method("CookingStation", "IsFireLit", 0, "assembly_valheim");
			MonoArray* slots          = nullptr;
			if (!slots_field || !get_slot || !set_slot || !get_conversion || !conversion_output || !conversion_time)
			{
				continue;
			}
			mono::get_field_value(object, slots_field, &slots);
			if (!slots)
			{
				continue;
			}

			for (int slot = 0; slot < mono::array_length(slots); ++slot)
			{
				MonoString* item_name = nullptr;
				float cooked_time     = 0.f;
				int status            = 0;
				bool cheated_out      = false;
				if (mono::get_param_count(get_slot) == 5)
				{
					void* get_args[] = {&slot, &item_name, &cooked_time, &status, &cheated_out};
					mono::invoke_method(get_slot, object, get_args);
				}
				else
				{
					void* get_args[] = {&slot, &item_name, &cooked_time, &status};
					mono::invoke_method(get_slot, object, get_args);
				}
				const std::string name = mono::from_mono_string(item_name);
				if (name.empty() || status != 0)
				{
					continue;
				}

				auto item_string        = mono::to_mono_string(name);
				void* conversion_args[] = {item_string};
				auto conversion         = mono::invoke_method(get_conversion, object, conversion_args);
				if (!conversion)
				{
					continue;
				}
				float duration = 0.f;
				mono::get_field_value(conversion, conversion_time, &duration);
				if (!(duration > 0.f))
				{
					continue;
				}
				MonoObject* output = nullptr;
				mono::get_field_value(conversion, conversion_output, &output);
				const std::string output_name = unity::get_name(output);
				if (!output || output_name.empty() || output_name == "unknown")
				{
					continue;
				}

				const float done_time = duration + std::max(0.01f, duration * 0.01f);
				auto output_string    = mono::to_mono_string(output_name);
				int done_status       = 1; // CookingStation.Status.Done
				bool cheated_in       = false;
				if (mono::get_param_count(set_slot) == 5)
				{
					void* set_args[] = {&slot, output_string, const_cast<float*>(&done_time), &done_status, &cheated_in};
					mono::invoke_method(set_slot, object, set_args);
				}
				else
				{
					void* set_args[] = {&slot, output_string, const_cast<float*>(&done_time), &done_status};
					mono::invoke_method(set_slot, object, set_args);
				}
				++count;
			}

			if (update_visual && is_fire_lit)
			{
				bool fire_lit       = value<bool>(object, "IsFireLit");
				void* visual_args[] = {&fire_lit};
				mono::invoke_method(update_visual, object, visual_args);
			}
		}
		for (auto object : nearby("SapCollector", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			int max_lvl     = field<int>(object, "m_maxLevel");
			void* args[]    = {&max_lvl};
			auto inc_method = find_method(mono::object_get_class(object), "IncreseLevel", 1);
			if (inc_method)
			{
				mono::invoke_method(inc_method, object, args);
				++count;
			}
		}
		notification::success("Instant Finish", std::format("Processed instant completion for {} production operation(s).", count));
		request_scan();
	}
	void base_tools::refuel_nearby_impl()
	{
		const auto cfg = get_options();
		int refueled   = 0;
		for (auto object : nearby("Smelter", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			int max_fuel = field<int>(object, "m_maxFuel");
			if (max_fuel > 0)
			{
				float fuel    = static_cast<float>(max_fuel);
				void* args[]  = {&fuel};
				auto set_fuel = find_method(mono::object_get_class(object), "SetFuel", 1);
				if (set_fuel)
				{
					mono::invoke_method(set_fuel, object, args);
					++refueled;
				}
			}
		}
		for (auto object : nearby("CookingStation", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			int max_fuel = field<int>(object, "m_maxFuel");
			if (max_fuel > 0)
			{
				float fuel    = static_cast<float>(max_fuel);
				void* args[]  = {&fuel};
				auto set_fuel = find_method(mono::object_get_class(object), "SetFuel", 1);
				if (set_fuel)
				{
					mono::invoke_method(set_fuel, object, args);
					++refueled;
				}
			}
		}
		for (auto object : nearby("Fireplace", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			float max_fuel = field<float>(object, "m_maxFuel");
			if (max_fuel > 0)
			{
				float fuel    = max_fuel;
				void* args[]  = {&fuel};
				auto set_fuel = find_method(mono::object_get_class(object), "SetFuel", 1);
				if (set_fuel)
				{
					mono::invoke_method(set_fuel, object, args);
					++refueled;
				}
			}
		}
		for (auto object : nearby("ShieldGenerator", cfg.radius))
		{
			if (!valid(object) || !access(position(object)))
			{
				continue;
			}
			if (!take_ownership(object))
			{
				continue;
			}
			int max_fuel = field<int>(object, "m_maxFuel");
			if (max_fuel > 0)
			{
				float fuel    = static_cast<float>(max_fuel);
				void* args[]  = {&fuel};
				auto set_fuel = find_method(mono::object_get_class(object), "SetFuel", 1);
				if (set_fuel)
				{
					mono::invoke_method(set_fuel, object, args);
					++refueled;
				}
			}
		}
		notification::success("Refuel Radius", std::format("Refueled {} nearby station(s) to 100%.", refueled));
		request_scan();
	}
	void base_tools::hotkey_tick_impl()
	{
		static bool was_down = false;
		bool down            = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
		if (down && !was_down && get_options().hotkey && g_pointers && g_pointers->m_hwnd
		    && GetForegroundWindow() == g_pointers->m_hwnd)
		{
			quick_stack();
		}
		was_down = down;
		std::vector<marker> next;
		if (get_options().damaged_markers)
		{
			auto state = get_snapshot();
			if (!state.buildings.empty() && esp_projection::begin_frame())
			{
				for (const auto& building : state.buildings)
				{
					Vector3 screen;
					if (esp_projection::project(building.position, screen))
					{
						next.push_back({building.detail, screen});
					}
					if (next.size() >= 200)
					{
						break;
					}
				}
			}
		}
		{
			std::lock_guard lock(snapshot_mutex);
			markers   = std::move(next);
			projected = std::chrono::steady_clock::now();
		}
	}
	void base_tools::advance_fermenters_impl(float speed, float radius)
	{
		static auto get_znet = mono::get_method("ZNet", "get_instance", 0, "assembly_valheim");
		static auto net_time = mono::get_field("ZNet", "m_netTime", "assembly_valheim");
		static auto get_zdo  = mono::get_method("ZNetView", "GetZDO", 0, "assembly_valheim");
		static auto get_long = mono::get_method_exact("ZDO", "GetLong", {"System.Int32", "System.Int64"}, "assembly_valheim");
		static auto set_long = mono::get_method_exact("ZDO", "Set", {"System.Int32", "System.Int64"}, "assembly_valheim");
		static auto start_time = static_cast<int*>(mono::get_static_field_value("ZDOVars", "s_startTime", "assembly_valheim"));
		std::unordered_map<MonoObject*, double> next;
		auto znet = get_znet ? mono::invoke_method(get_znet, nullptr, nullptr) : nullptr;
		if (speed > 1.f && znet && net_time && get_zdo && get_long && set_long && start_time)
		{
			double now = 0.0;
			mono::get_field_value(znet, net_time, &now);
			for (auto object : nearby("Fermenter", radius))
			{
				auto view = field<MonoObject*>(object, "m_nview");
				if (!valid(object) || !value<bool>(view, "IsOwner"))
				{
					continue;
				}
				if (auto previous = fermenter_clock.find(object); previous != fermenter_clock.end() && now > previous->second)
				{
					auto zdo          = mono::invoke_method(get_zdo, view, nullptr);
					int key           = *start_time;
					int64_t empty     = 0;
					void* get_args[2] = {&key, &empty};
					auto boxed        = zdo ? mono::invoke_method(get_long, zdo, get_args) : nullptr;
					int64_t start     = boxed ? *static_cast<int64_t*>(mono::object_unbox(boxed)) : 0;
					if (start != 0)
					{
						int64_t shifted = start - static_cast<int64_t>((speed - 1.f) * (now - previous->second) * 10000000.0);
						void* set_args[2] = {&key, &shifted};
						mono::invoke_method(set_long, zdo, set_args);
					}
				}
				next[object] = now;
			}
		}
		fermenter_clock = std::move(next);
	}
	void base_tools::update_impl()
	{
		const auto cfg = get_options();
		advance_fermenters_impl(cfg.fermenter_speed, cfg.radius);
		if (!scan_requested.exchange(false) && !cfg.damaged_markers)
		{
			return;
		}
		base_tools::snapshot next;
		auto local = unity::get_local_player();
		if (local)
		{
			next.ready = true;
			for (auto object : nearby("Smelter", cfg.radius))
			{
				if (valid(object))
				{
					int queue          = value<int>(object, "GetQueueSize");
					int max_ore        = field<int>(object, "m_maxOre");
					int max_fuel       = field<int>(object, "m_maxFuel");
					float fuel         = value<float>(object, "GetFuel");
					float speed        = multiplier(object, cfg.smelter_speed);
					float bake_timer   = value<float>(object, "GetBakeTimer");
					float sec_per_prod = field<float>(object, "m_secPerProduct");
					int processed      = value<int>(object, "GetProcessedQueueSize");

					std::string detail;
					float progress_pct = (sec_per_prod > 0.f && queue > 0) ? std::clamp((bake_timer / sec_per_prod) * 100.f, 0.f, 100.f) : 0.f;

					if (max_fuel <= 0)
					{
						if (queue > 0)
						{
							detail = std::format("Queue: {}/{} ({:.0f}%) | Ready: {} | Speed: {:.1f}x", queue, max_ore, progress_pct, processed, speed);
						}
						else
						{
							detail = std::format("Queue: 0/{} (Empty) | Ready: {} | Speed: {:.1f}x", max_ore, processed, speed);
						}
					}
					else
					{
						if (queue > 0)
						{
							detail = std::format("Ore: {}/{} ({:.0f}%) | Fuel: {:.0f}/{} | Ready: {} | Speed: {:.1f}x", queue, max_ore, progress_pct, fuel, max_fuel, processed, speed);
						}
						else
						{
							detail = std::format("Ore: 0/{} | Fuel: {:.0f}/{} | Ready: {} | Speed: {:.1f}x", max_ore, fuel, max_fuel, processed, speed);
						}
					}

					next.production.push_back({name(object), detail, position(object)});
				}
			}
			for (auto object : nearby("CookingStation", cfg.radius))
			{
				if (valid(object))
				{
					float fuel      = value<float>(object, "GetFuel");
					int max_fuel    = field<int>(object, "m_maxFuel");
					bool fire       = value<bool>(object, "IsFireLit");
					float speed     = multiplier(object, cfg.cooking_speed);
					int total_slots = 0;
					auto slots      = field<MonoArray*>(object, "m_slots");
					if (slots)
					{
						total_slots = mono_array_view<MonoObject*>(slots).size();
					}

					std::string detail;
					if (max_fuel > 0)
					{
						detail = std::format("Slots: {} | Fire: {} | Fuel: {:.0f}/{} | Speed: {:.1f}x", total_slots, fire ? "Lit" : "No Fire", fuel, max_fuel, speed);
					}
					else
					{
						detail = std::format("Slots: {} | Fire: {} | Speed: {:.1f}x", total_slots, fire ? "Lit" : "No Fire", speed);
					}

					next.production.push_back({name(object), detail, position(object)});
				}
			}
			for (auto object : nearby("SapCollector", cfg.radius))
			{
				if (valid(object))
				{
					int level   = value<int>(object, "GetLevel");
					int max_lvl = field<int>(object, "m_maxLevel");
					float speed = multiplier(object, cfg.sap_speed);
					auto status_str = mono::from_mono_string(reinterpret_cast<MonoString*>(call(object, "GetStatusText")));
					if (status_str.empty())
					{
						status_str = (level >= max_lvl) ? "Full" : "Extracting";
					}

					std::string detail = std::format("Sap: {}/{} ({}) | Speed: {:.1f}x", level, max_lvl, status_str, speed);
					next.production.push_back({name(object), detail, position(object)});
				}
			}
			for (auto object : nearby("ShieldGenerator", cfg.radius))
			{
				if (valid(object))
				{
					float fuel   = value<float>(object, "GetFuel");
					int max_fuel = field<int>(object, "m_maxFuel");
					float charge = value<float>(object, "GetAttackCharge") * 100.f;
					float min_r  = field<float>(object, "m_minShieldRadius");
					float max_r  = field<float>(object, "m_maxShieldRadius");
					std::string detail = std::format("Fuel: {:.0f}/{} | Charge: {:.0f}% | Radius: {:.0f}-{:.0f}m", fuel, max_fuel, charge, min_r, max_r);
					next.production.push_back({name(object), detail, position(object)});
				}
			}
			for (auto object : nearby("Fermenter", cfg.radius))
			{
				if (valid(object))
				{
					int status           = value<int>(object, "GetStatus");
					const char* states[] = {"Empty", "Fermenting", "Exposed", "Ready"};
					double remaining = std::max(0., double(field<float>(object, "m_fermentationDuration")) - value<double>(object, "GetFermentationTime"));
					next.production.push_back({name(object), std::format("{} | Remaining: {:.0f}s", status >= 0 && status < 4 ? states[status] : "Unknown", status == 1 ? remaining / multiplier(object, cfg.fermenter_speed) : 0.), position(object)});
				}
			}
			for (auto object : nearby("Beehive", cfg.radius))
			{
				if (valid(object))
				{
					next.production.push_back({name(object), std::format("Honey: {} / {} | Speed: {:.1f}x", value<int>(object, "GetHoneyLevel"), field<int>(object, "m_maxHoney"), multiplier(object, cfg.honey_speed)), position(object)});
				}
			}
			for (auto object : nearby("Plant", cfg.radius))
			{
				if (valid(object))
				{
					auto status = mono::from_mono_string(reinterpret_cast<MonoString*>(call(object, "GetHoverText")));
					double remaining = std::max(0., double(value<float>(object, "GetGrowTime")) - value<double>(object, "TimeSincePlanted"));
					int plant_status = value<int>(object, "GetStatus");
					const char* status_names[] = {"Healthy", "No Grow", "No Sun", "No Space", "Too Cold (Deep North/Mtn)", "Too Hot (Ashlands)", "Wrong Biome", "Not Cultivated", "No Attach Piece"};
					const char* status_lbl = (plant_status >= 0 && plant_status <= 8) ? status_names[plant_status] : "Unknown";
					float speed = multiplier(object, cfg.plant_speed);
					std::string detail = std::format("Status: {} | ETA: {:.0f}s | Speed: {:.1f}x", status_lbl, (speed > 1.f ? remaining / speed : remaining), speed);
					next.plants.push_back({name(object), detail, position(object)});
				}
			}
			for (auto object : nearby("WearNTear", cfg.radius))
			{
				if (valid(object))
				{
					float health = value<float>(object, "GetHealthPercentage");
					if (health < .999f)
					{
						next.buildings.push_back({name(object), std::format("Health: {:.0f}%", health * 100.f), position(object)});
					}
				}
			}
			next.sheltered      = value<bool>(local, "InShelter");
			next.comfort_level  = value<int>(local, "GetComfortLevel");
			static auto comfort = mono::get_method("SE_Rested", "GetNearbyComfortPieces", 1, "assembly_valheim");
			auto origin         = player(local).get_position();
			void* args[]        = {&origin};
			auto pieces         = mono::invoke_method(comfort, nullptr, args);
			// This is the game's own comfort radius, independent of the action radius.
			for (auto piece : mono::list<MonoObject*>(pieces))
			{
				next.comfort.push_back({name(piece), std::format("Comfort: {} | Group: {} (duplicates may not stack)", value<int>(piece, "GetComfort"), field<int>(piece, "m_comfortGroup")), position(piece)});
			}
			static auto env_get = mono::get_method("EnvMan", "get_instance", 0, "assembly_valheim");
			auto env            = mono::invoke_method(env_get);
			if (env)
			{
				for (auto setup : mono::list<MonoObject*>(field<MonoObject*>(env, "m_environments")))
				{
					auto label = mono::from_mono_string(field<MonoString*>(setup, "m_name"));
					if (!label.empty() && std::find(next.weather.begin(), next.weather.end(), label) == next.weather.end())
					{
						next.weather.push_back(label);
					}
				}
				for (auto biome : mono::list<MonoObject*>(field<MonoObject*>(env, "m_biomes")))
				{
					for (auto entry : mono::list<MonoObject*>(field<MonoObject*>(biome, "m_environments")))
					{
						auto setup = field<MonoObject*>(entry, "m_env");
						if (setup)
						{
							auto label = mono::from_mono_string(field<MonoString*>(setup, "m_name"));
							if (!label.empty()
							    && std::find(next.weather.begin(), next.weather.end(), label) == next.weather.end())
							{
								next.weather.push_back(label);
							}
						}
					}
				}
				for (const std::string& dn_env : {"DeepNorth", "DeepNorthSnow", "DeepNorthClear", "Ashlands", "Mistlands"})
				{
					if (std::find(next.weather.begin(), next.weather.end(), dn_env) == next.weather.end())
					{
						next.weather.push_back(dn_env);
					}
				}
			}
		}
		{
			std::lock_guard lock(snapshot_mutex);
			data    = std::move(next);
			scanned = std::chrono::steady_clock::now();
		}
	}
}
