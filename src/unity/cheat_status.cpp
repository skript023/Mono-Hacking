#include "cheat_status.hpp"
#include "hooking.hpp"
#include "task_mgr.hpp"
#include "utility/unity.hpp"
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace big::cheat_status
{
	namespace
	{
		std::mutex mutex;
		snapshot published;
		std::atomic_bool pending = false;
		std::string last_session;

		MonoClassField* field_info(MonoObject* object, const char* name)
		{
			if (!object) throw std::runtime_error(std::string("Missing object for ") + name);
			for (auto k = mono::object_get_class(object); k; k = mono::class_get_parent(k))
				if (auto f = mono::get_field(k, name)) return f;
			throw std::runtime_error(std::string("Missing field: ") + name);
		}
		template<typename T> T field(MonoObject* object, const char* name)
		{
			T result{};
			mono::get_field_value(object, field_info(object, name), &result);
			return result;
		}
		template<typename T> void set_field(MonoObject* object, const char* name, T value)
		{
			mono::set_field_value(object, field_info(object, name), &value);
		}
		MonoMethod* method(const char* type, const char* name, std::initializer_list<const char*> params = {})
		{
			auto m = mono::get_method_exact(type, name, params, "assembly_valheim");
			if (!m) throw std::runtime_error(std::string("Missing method: ") + type + "." + name);
			return m;
		}
		MonoObject* invoke(MonoMethod* m, MonoObject* object = nullptr, void** args = nullptr)
		{
			if (!m) throw std::runtime_error("Required game API unavailable");
			MonoObject* exception = nullptr;
			auto result = mono::invoke_method(m, object, args, &exception);
			if (exception) throw std::runtime_error("Game API raised an exception; inspection/action is incomplete");
			return result;
		}
		MonoObject* call(MonoObject* object, const char* type, const char* name)
		{
			if (!object) throw std::runtime_error(std::string("Missing object: ") + type);
			return invoke(method(type, name), object);
		}
		template<typename T> T unbox(MonoObject* object)
		{
			if (!object) throw std::runtime_error("Missing game return value");
			return *static_cast<T*>(mono::object_unbox(object));
		}
		std::vector<MonoObject*> list(MonoObject* object)
		{
			auto items = field<MonoArray*>(object, "_items");
			auto size = field<int>(object, "_size");
			if (!items || size < 0 || size > mono::array_length(items)) throw std::runtime_error("Invalid game list");
			std::vector<MonoObject*> result;
			for (auto item : mono_array_view<MonoObject*>(items, size)) result.push_back(item);
			return result;
		}
		MonoObject* component(MonoObject* object, const char* type)
		{
			auto klass = mono::get_class(type, "assembly_valheim");
			if (!klass) throw std::runtime_error(std::string("Missing component: ") + type);
			void* args[] = {mono::reflection_type(klass)};
			return invoke(mono::get_method_exact("Component", "GetComponent", {"System.Type"}, "UnityEngine.CoreModule", "UnityEngine"), object, args);
		}
		int inventory(MonoObject* object, bool clean)
		{
			int count = 0;
			for (auto item : list(field<MonoObject*>(object, "m_inventory")))
			{
				if (!item) throw std::runtime_error("Missing inventory item");
				if (field<bool>(item, "m_cheated"))
				{
					++count;
					if (clean) set_field(item, "m_cheated", false);
				}
			}
			if (clean && count)
			{
				bool no = false;
				void* args[] = {&no, &no};
				invoke(method("Inventory", "Changed", {"System.Boolean", "System.Boolean"}), object, args);
			}
			return count;
		}
		int zdo_hash(const char* name)
		{
			auto klass = mono::get_class("ZDOVars", "assembly_valheim");
			auto f = klass ? mono::get_field(klass, name) : nullptr;
			if (!f) throw std::runtime_error(std::string("Missing ZDO key: ") + name);
			auto data = static_cast<const char*>(mono::get_static_field_data(klass));
			if (!data) throw std::runtime_error("ZDO key storage unavailable");
			int result{};
			std::memcpy(&result, data + mono::get_field_offset(f), sizeof(result));
			if (!result) throw std::runtime_error("ZDO keys are not initialized");
			return result;
		}
		bool zdo_flag(MonoObject* zdo, int key, bool clean)
		{
			bool no = false;
			void* args[] = {&key, &no};
			bool marked = unbox<bool>(invoke(method("ZDO", "GetBool", {"System.Int32", "System.Boolean"}), zdo, args));
			if (marked && clean) invoke(method("ZDO", "Set", {"System.Int32", "System.Boolean"}), zdo, args);
			return marked;
		}
		std::vector<std::string> world_keys(MonoObject* world)
		{
			std::vector<std::string> allowed;
			for (auto key : list(invoke(method("ServerOptionsGUI", "GetPossibleGlobalKeyValues"))))
				allowed.push_back(mono::from_mono_string(reinterpret_cast<MonoString*>(key)));
			std::vector<std::string> result;
			for (auto key : list(field<MonoObject*>(world, "m_startingGlobalKeys")))
			{
				auto text = mono::from_mono_string(reinterpret_cast<MonoString*>(key));
				if (!text.starts_with("preset") && std::find(allowed.begin(), allowed.end(), text) == allowed.end()) result.push_back(text);
			}
			return result;
		}
		void inspect_area(snapshot& state, MonoObject* local, bool clean)
		{
			const int cheated = zdo_hash("s_cheated"), queued = zdo_hash("s_cheatedQueued");
			void* args[] = {mono::reflection_type(mono::get_class("ZNetView", "assembly_valheim"))};
			auto objects = reinterpret_cast<MonoArray*>(invoke(mono::get_method_exact("Object", "FindObjectsOfType", {"System.Type"}, "UnityEngine.CoreModule", "UnityEngine"), nullptr, args));
			if (!objects) throw std::runtime_error("Loaded objects unavailable");
			const auto origin = unity::get_position(local);
			for (auto view : mono_array_view<MonoObject*>(objects))
			{
				if (!view || !unbox<bool>(call(view, "ZNetView", "IsValid"))) continue;
				auto delta = unity::get_position(view) - origin;
				if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > state.radius * state.radius) continue;
				auto zdo = call(view, "ZNetView", "GetZDO");
				bool owner = unbox<bool>(call(view, "ZNetView", "IsOwner"));
				int flags = zdo_flag(zdo, cheated, clean && owner) + zdo_flag(zdo, queued, clean && owner);
				auto cooker = component(view, "CookingStation");
				auto fermenter = component(view, "Fermenter");
				if (cooker)
				{
					auto slots = field<MonoArray*>(cooker, "m_slots");
					if (!slots) throw std::runtime_error("Cooking slots unavailable");
					for (int i = 1; i < mono::array_length(slots); ++i) flags += zdo_flag(zdo, queued + i, clean && owner);
				}
				if (fermenter)
				{
					if (field<bool>(fermenter, "m_delayedTapItemCheated"))
					{
						++flags;
						if (clean && owner) set_field(fermenter, "m_delayedTapItemCheated", false);
					}
				}
				if (flags)
				{
					++state.marked_objects;
					if (component(view, "Character")) ++state.marked_creatures;
					if (component(view, "Piece")) ++state.marked_pieces;
					if (cooker || fermenter || component(view, "Smelter")) ++state.marked_stations;
					state.marked_flags += flags;
					if (!owner) ++state.skipped_objects;
				}
				if (auto chest = component(view, "Container"))
				{
					if (!owner || unbox<bool>(call(chest, "Container", "IsInUse")))
					{
						++state.uninspected_chests;
						continue;
					}
					++state.chests;
					int count = inventory(field<MonoObject*>(chest, "m_inventory"), clean);
					state.chest_items += count;
					if (clean && count) call(chest, "Container", "Save");
				}
			}
		}
		void run(action operation, float radius)
		{
			snapshot state;
			state.radius = radius;
			{ std::lock_guard lock(mutex); state.result = published.result; }
			try
			{
				auto local = unity::get_local_player();
				if (!local)
				{
					last_session.clear();
					state.result.clear();
					throw std::runtime_error("Join a world to inspect cheat flags");
				}
				auto game = invoke(method("Game", "get_instance"));
				auto profile = call(game, "Game", "GetPlayerProfile");
				state.character_marked = field<bool>(profile, "m_usedCheats");
				state.character_ready = true;
				auto net = invoke(method("ZNet", "get_instance"));
				auto world = invoke(method("ZNet", "get_World"));
				auto session = std::to_string(field<int64_t>(world, "m_uid")) + ":" + mono::from_mono_string(field<MonoString*>(profile, "m_filename"));
				if (session != last_session) state.result.clear();
				last_session = std::move(session);
				state.host = unbox<bool>(call(net, "ZNet", "IsServer"));
				if ((operation == action::world || operation == action::nearby) && !state.host)
					throw std::runtime_error("World/area cleanup requires the host; join locally or run on the host");
				state.world_keys = world_keys(world);
				if (operation == action::character)
				{
					set_field(profile, "m_usedCheats", false);
					state.character_marked = field<bool>(profile, "m_usedCheats");
					auto stats = field<MonoArray*>(profile, "m_playerStats");
					if (!stats) throw std::runtime_error("Character statistics unavailable");
					int cheats_stat = 4;
					float zero = 0.f;
					void* args[] = {&cheats_stat, &zero};
					for (auto entry : mono_array_view<MonoObject*>(stats))
						if (entry) invoke(method("PlayerProfile/PlayerStats", "set_Item", {"PlayerStatType", "System.Single"}), entry, args);
					inventory(call(local, "Humanoid", "GetInventory"), true);
					bool no = false;
					void* save_args[] = {&no, &no};
					invoke(method("Game", "SavePlayerProfile", {"System.Boolean", "System.Boolean"}), game, save_args);
					state.result = "Character/inventory flags cleared. Character save requested; check the game's save result.";
				}
				if (operation == action::world)
				{
					auto zone = invoke(method("ZoneSystem", "get_instance"));
					auto keys = field<MonoObject*>(world, "m_startingGlobalKeys");
					auto remove = mono::class_get_method_from_name(mono::object_get_class(keys), "Remove", 1);
					for (const auto& key : state.world_keys)
					{
						auto text = mono::to_mono_string(key);
						bool yes = true;
						void* args[] = {text, &yes};
						invoke(method("ZoneSystem", "GlobalKeyRemove", {"System.String", "System.Boolean"}), zone, args);
						while (unbox<bool>(invoke(remove, keys, args))) {}
					}
					int64_t everyone = 0;
					void* args[] = {&everyone};
					invoke(method("ZoneSystem", "SendGlobalKeys", {"System.Int64"}), zone, args);
					state.result = "Cheat world modifiers removed (their gameplay effects are reset). World save requested; check the game's save result.";
					state.world_keys = world_keys(world);
				}
				if (operation == action::nearby)
				{
					snapshot before;
					before.radius = radius;
					inspect_area(before, local, true);
					state.result = std::format("Area cleanup applied. {} non-owned marked objects and {} busy/non-owned chests skipped. World save requested; check the game's save result.", before.skipped_objects, before.uninspected_chests);
				}
				if (operation == action::world || operation == action::nearby) call(net, "ZNet", "SaveWorldAndPlayerProfiles");
				state.character_marked = field<bool>(profile, "m_usedCheats");
				state.inventory_items = inventory(call(local, "Humanoid", "GetInventory"), false);
				state.native_bypass = detour_base::get_original<hooks::bypass_cheat_checks>()();
				state.mod_bypass = g_running;
				inspect_area(state, local, false);
				state.ready = true;
			}
			catch (const std::exception& e)
			{
				state.error = e.what();
				if (operation != action::inspect) state.result = "Cleanup incomplete; some changes may already be applied. Inspect again before saving/exiting.";
			}
			state.sampled = std::chrono::steady_clock::now();
			{ std::lock_guard lock(mutex); published = std::move(state); }
			if (operation == action::inspect) pending = false;
		}
	}
	snapshot get_snapshot()
	{
		std::lock_guard lock(mutex);
		return published;
	}
	void request(action operation, float radius)
	{
		if (operation == action::inspect)
		{
			auto state = get_snapshot();
			if (state.radius == radius && std::chrono::steady_clock::now() - state.sampled < std::chrono::seconds(1)) return;
			if (pending.exchange(true)) return;
		}
		async_run([operation, radius = std::clamp(radius, 5.f, 100.f)] { run(operation, radius); });
	}
}
