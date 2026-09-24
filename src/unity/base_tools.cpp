#include "base_tools.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "unity/localization.hpp"
#include "utility/unity.hpp"
#include "pointers.hpp"
#include "notification/notification_service.hpp"
#include <mutex>

namespace big::base_tools
{
	namespace
	{
		std::mutex mutex;
		options config;
		snapshot data;
		std::atomic_bool scan_requested = false;
		std::chrono::steady_clock::time_point scanned;
		std::vector<marker> markers;
		std::chrono::steady_clock::time_point projected;
		std::unordered_map<int, std::chrono::steady_clock::time_point> pending_stacks;

		template<typename T> T field(MonoObject* object, const char* name)
		{
			T value{};
			if (object)
				if (auto f = mono::get_field(mono::object_get_class(object), name)) mono::get_field_value(object, f, &value);
			return value;
		}
		MonoObject* call(MonoObject* object, const char* name)
		{
			if (!object) return nullptr;
			return mono::invoke_method(mono::class_get_method_from_name(mono::object_get_class(object), name, 0), object);
		}
		template<typename T> T value(MonoObject* object, const char* name)
		{
			auto result = call(object, name);
			return result ? *static_cast<T*>(mono::object_unbox(result)) : T{};
		}
		bool valid(MonoObject* object)
		{
			auto view = field<MonoObject*>(object, "m_nview");
			return view && value<bool>(view, "IsValid");
		}
		Vector3 position(MonoObject* object)
		{
			return value<Vector3>(call(object, "get_transform"), "get_position");
		}
		float distance_squared(Vector3 a, Vector3 b)
		{
			auto d = a - b;
			return d.x*d.x + d.y*d.y + d.z*d.z;
		}
		std::vector<MonoObject*> nearby(const char* name, float radius)
		{
			std::vector<MonoObject*> result;
			auto local = unity::get_local_player();
			if (!local) return result;
			static auto find = mono::get_method_overload("Object", "FindObjectsOfType", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
			auto type = mono::reflection_type(mono::get_class(name, "assembly_valheim"));
			if (!find || !type) return result;
			void* args[] = { type };
			auto array = reinterpret_cast<MonoArray*>(mono::invoke_method(find, nullptr, args));
			if (!array) return result;
			const auto origin = player(local).get_position();
			for (auto object : mono_array_view<MonoObject*>(array))
				if (object && distance_squared(origin, position(object)) <= radius*radius) result.push_back(object);
			std::sort(result.begin(), result.end(), [&](auto a, auto b) { return distance_squared(origin, position(a)) < distance_squared(origin, position(b)); });
			return result;
		}
		std::string name(MonoObject* object)
		{
			auto token = mono::from_mono_string(field<MonoString*>(object, "m_name"));
			if (token.empty()) token = mono::from_mono_string(reinterpret_cast<MonoString*>(call(object, "get_name")));
			return localization::get_instance().localize(token);
		}
		bool access(Vector3 point)
		{
			static auto method = mono::get_method("PrivateArea", "CheckAccess", 4, "assembly_valheim");
			float radius = 0.f; bool flash = false, ward = true;
			void* args[] = { &point, &radius, &flash, &ward };
			auto result = mono::invoke_method(method, nullptr, args);
			return result && *static_cast<bool*>(mono::object_unbox(result));
		}
	}

	options get_options() { std::lock_guard lock(mutex); return config; }
	void set_options(options v)
	{
		v.radius = std::clamp(v.radius, 5.f, 100.f);
		for (auto p : { &v.smelter_speed, &v.fermenter_speed, &v.honey_speed, &v.plant_speed }) *p = std::clamp(*p, 1.f, 20.f);
		v.daylight = std::clamp(v.daylight, 0.f, 1.f);
		std::lock_guard lock(mutex); config = std::move(v);
	}
	snapshot get_snapshot()
	{
		std::lock_guard lock(mutex);
		if (std::chrono::steady_clock::now() - scanned > std::chrono::seconds(5)) return {};
		return data;
	}
	void request_scan() { scan_requested = true; }
	std::vector<marker> get_markers()
	{
		std::lock_guard lock(mutex);
		return std::chrono::steady_clock::now() - projected < std::chrono::milliseconds(500) ? markers : std::vector<marker>{};
	}
	float multiplier(MonoObject* object, float speed)
	{
		if (speed <= 1.f || !object || !valid(object)) return 1.f;
		auto local = unity::get_local_player();
		if (!local || !value<bool>(field<MonoObject*>(object, "m_nview"), "IsOwner")) return 1.f;
		const float radius = get_options().radius;
		return distance_squared(position(object), player(local).get_position()) <= radius*radius ? speed : 1.f;
	}

	void quick_stack()
	{
		const auto cfg = get_options();
		int requested = 0;
		for (auto chest : nearby("Container", cfg.radius))
		{
			if (!valid(chest) || value<bool>(chest, "IsInUse") || !access(position(chest))) continue;
			const int id = value<int>(chest, "GetInstanceID");
			if (pending_stacks.contains(id)) continue;
			pending_stacks[id] = std::chrono::steady_clock::now();
			call(chest, "StackAll");
			++requested;
		}
		notification::info("Quick Stack", std::format("Requested stacking into {} nearby chests. Access and capacity are checked by the game.", requested));
	}
	bool begin_stack_response(MonoObject* chest)
	{
		auto it = pending_stacks.find(value<int>(chest, "GetInstanceID"));
		if (it == pending_stacks.end()) return false;
		pending_stacks.erase(it);
		// Keep protection even if the server answers a request late.
		return true;
	}
	bool protected_item(MonoObject* item)
	{
		if (!item) return true;
		const auto cfg = get_options();
		auto shared = field<MonoObject*>(item, "m_shared");
		int type = field<int>(shared, "m_itemType");
		return (cfg.keep_hotbar && field<iVector2>(item, "m_gridPos").y == 0)
			|| (cfg.keep_food && type == 2) || (cfg.keep_ammo && (type == 9 || type == 23));
	}
	void repair_nearby()
	{
		int count = 0;
		for (auto piece : nearby("WearNTear", get_options().radius))
			if (valid(piece) && access(position(piece)) && value<bool>(piece, "Repair")) ++count;
		notification::info("Repair Radius", std::format("Sent {} repair requests.", count));
		request_scan();
	}
	void grow_nearby()
	{
		int count = 0;
		for (auto plant : nearby("Plant", get_options().radius))
		{
			if (multiplier(plant, 2.f) == 1.f || !access(position(plant))) continue;
			double elapsed = value<double>(plant, "TimeSincePlanted");
			void* args[] = { &elapsed };
			auto method = mono::class_get_method_from_name(mono::object_get_class(plant), "UpdateHealth", 1);
			mono::invoke_method(method, plant, args);
			if (value<int>(plant, "GetStatus") == 0 && call(plant, "Grow")) ++count;
		}
		notification::info("Farming", std::format("Grew {} healthy, locally owned plants.", count));
		request_scan();
	}
	void hotkey_tick()
	{
		static bool was_down = false;
		bool down = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
		if (down && !was_down && get_options().hotkey && GetForegroundWindow() == g_pointers->m_hwnd) quick_stack();
		was_down = down;
		std::vector<marker> next;
		if (get_options().damaged_markers)
		{
			auto state = get_snapshot();
			for (const auto& building : state.buildings)
			{
				Vector3 screen;
				if (unity::world_to_screen(building.position, screen)) next.push_back({building.detail, screen});
				if (next.size() >= 200) break;
			}
		}
		std::lock_guard lock(mutex); markers = std::move(next); projected = std::chrono::steady_clock::now();
	}
	void update()
	{
		const auto cfg = get_options();
		if (!scan_requested.exchange(false) && !cfg.damaged_markers) return;
		snapshot next;
		auto local = unity::get_local_player();
		if (local)
		{
			next.ready = true;
			for (auto object : nearby("Smelter", cfg.radius))
				if (valid(object)) next.production.push_back({name(object), std::format("Queue: {} | Fuel: {:.1f} | Speed: {:.1f}x", value<int>(object,"GetQueueSize"), value<float>(object,"GetFuel"), multiplier(object,cfg.smelter_speed)), position(object)});
			for (auto object : nearby("Fermenter", cfg.radius))
				if (valid(object))
				{
					int status = value<int>(object,"GetStatus");
					const char* states[] = {"Empty", "Fermenting", "Exposed", "Ready"};
					double remaining = std::max(0., double(field<float>(object,"m_fermentationDuration")) - value<double>(object,"GetFermentationTime"));
					next.production.push_back({name(object), std::format("{} | Remaining: {:.0f}s", status>=0 && status<4 ? states[status] : "Unknown", status == 1 ? remaining / multiplier(object,cfg.fermenter_speed) : 0.), position(object)});
				}
			for (auto object : nearby("Beehive", cfg.radius))
				if (valid(object)) next.production.push_back({name(object), std::format("Honey: {} / {} | Speed: {:.1f}x", value<int>(object,"GetHoneyLevel"), field<int>(object,"m_maxHoney"), multiplier(object,cfg.honey_speed)), position(object)});
			for (auto object : nearby("Plant", cfg.radius))
				if (valid(object))
				{
					auto status = mono::from_mono_string(reinterpret_cast<MonoString*>(call(object,"GetHoverText")));
					double remaining = std::max(0., double(value<float>(object,"GetGrowTime")) - value<double>(object,"TimeSincePlanted"));
					next.plants.push_back({name(object), std::format("{} | ETA: {:.0f}s (if healthy)", status, remaining), position(object)});
				}
			for (auto object : nearby("WearNTear", cfg.radius))
				if (valid(object))
				{
					float health = value<float>(object,"GetHealthPercentage");
					if (health < .999f) next.buildings.push_back({name(object), std::format("Health: {:.0f}%", health*100.f), position(object)});
				}
			next.sheltered = value<bool>(local, "InShelter");
			next.comfort_level = value<int>(local, "GetComfortLevel");
			static auto comfort = mono::get_method("SE_Rested", "GetNearbyComfortPieces", 1, "assembly_valheim");
			auto origin = player(local).get_position(); void* args[] = { &origin };
			auto pieces = mono::invoke_method(comfort, nullptr, args);
			// This is the game's own comfort radius, independent of the action radius.
			for (auto piece : mono::list<MonoObject*>(pieces))
				next.comfort.push_back({name(piece), std::format("Comfort: {} | Group: {} (duplicates may not stack)", value<int>(piece,"GetComfort"), field<int>(piece,"m_comfortGroup")), position(piece)});
			static auto env_get = mono::get_method("EnvMan", "get_instance", 0, "assembly_valheim");
			auto env = mono::invoke_method(env_get);
			for (auto setup : mono::list<MonoObject*>(field<MonoObject*>(env,"m_environments")))
			{
				auto label = mono::from_mono_string(field<MonoString*>(setup,"m_name"));
				if (!label.empty()) next.weather.push_back(label);
			}
		}
		std::lock_guard lock(mutex); data = std::move(next); scanned = std::chrono::steady_clock::now();
	}
}
