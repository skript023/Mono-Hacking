#include "online_players.hpp"
#include "mono/mono.hpp"
#include "unity/player.hpp"
#include "utility/unity.hpp"
#include <mutex>

namespace big::online_players
{
	namespace
	{
		std::mutex mutex;
		snapshot current;
		std::chrono::steady_clock::time_point last_update;

		template<typename T>
		T field(MonoObject* object, const char* name)
		{
			T value{};
			if (object)
				if (auto member = mono::get_field(mono::object_get_class(object), name))
					mono::get_field_value(object, member, &value);
			return value;
		}

		std::string zdo_id(MonoObject* boxed)
		{
			if (!boxed)
				return {};
			auto method = mono::class_get_method_from_name(mono::object_get_class(boxed), "ToString", 0);
			auto text = mono::invoke_method(method, mono::object_unbox(boxed));
			return text ? mono::from_mono_string(reinterpret_cast<MonoString*>(text)) : std::string{};
		}

		MonoObject* boxed_member(MonoObject* object, const char* name)
		{
			return object ? mono::boxed_field(object, mono::get_field(mono::object_get_class(object), name)) : nullptr;
		}

		std::string character_id(player value)
		{
			static auto method = mono::get_method("Character", "GetZDOID", 0, "assembly_valheim");
			return zdo_id(mono::invoke_method(method, value.get_object()));
		}

		snapshot collect()
		{
			snapshot result;
			auto local = unity::get_local_player();
			if (!local)
				return result;
			static auto instance_method = mono::get_method("ZNet", "get_instance", 0, "assembly_valheim");
			static auto roster_method = mono::get_method("ZNet", "GetPlayerList", 0, "assembly_valheim");
			auto network = mono::invoke_method(instance_method);
			if (!network)
				return result;
			auto roster = mono::invoke_method(roster_method, network);
			if (!roster)
				return result;
			auto klass = mono::object_get_class(roster);
			auto count_method = mono::class_get_method_from_name(klass, "get_Count", 0);
			auto item_method = mono::class_get_method_from_name(klass, "get_Item", 1);
			auto count_box = mono::invoke_method(count_method, roster);
			if (!count_box || !item_method)
				return result;
			int count = *static_cast<int*>(mono::object_unbox(count_box));
			if (count < 0 || count > 4096)
				return result;
			result.ready = true;
			auto local_player = player(local);
			const auto local_id = character_id(local_player);
			const auto local_position = local_player.get_position();
			std::unordered_map<std::string, MonoObject*> loaded;
			for (auto value : player::get_all_players())
				if (value)
					loaded.emplace(character_id(value), value.get_object());

			for (int index = 0; index < count; ++index)
			{
				void* args[] = {&index};
				auto info = mono::invoke_method(item_method, roster, args);
				if (!info)
					continue;
				auto id_field = mono::get_field(mono::object_get_class(info), "m_characterID");
				entry value;
				value.id = zdo_id(mono::boxed_field(info, id_field));
				value.name = mono::from_mono_string(field<MonoString*>(info, "m_name"));
				auto user_info = boxed_member(info, "m_userInfo");
				auto platform_id = boxed_member(user_info, "m_id");
				auto platform = boxed_member(platform_id, "m_platform");
				value.platform = mono::from_mono_string(field<MonoString*>(platform, "m_platform"));
				value.account_id = mono::from_mono_string(field<MonoString*>(platform_id, "m_userID"));
				value.public_position = field<bool>(info, "m_publicPosition");
				value.local = !local_id.empty() && local_id != "0:0" && value.id == local_id;
				if (value.public_position)
					value.position = field<Vector3>(info, "m_position");
				if (auto found = loaded.find(value.id); found != loaded.end() && !value.id.empty() && value.id != "0:0")
				{
					player entity(found->second);
					value.loaded = true;
					value.position = entity.get_position();
					value.health = entity.get_health();
					value.max_health = entity.get_max_health();
				}
				if (value.position)
				{
					auto delta = *value.position - local_position;
					value.distance = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
				}
				result.players.push_back(std::move(value));
			}
			std::sort(result.players.begin(), result.players.end(), [](const entry& a, const entry& b) {
				if (a.local != b.local)
					return a.local;
				return a.name == b.name ? a.id < b.id : a.name < b.name;
			});
			return result;
		}
	}

	void update()
	{
		auto next = collect();
		std::lock_guard lock(mutex);
		current = std::move(next);
		last_update = std::chrono::steady_clock::now();
	}

	snapshot get_snapshot()
	{
		std::lock_guard lock(mutex);
		if (std::chrono::steady_clock::now() - last_update > std::chrono::seconds(3))
			return {};
		return current;
	}

	bool teleport_to(const std::string& id)
	{
		if (id.empty() || id == "0:0")
			return false;
		auto latest = collect();
		for (const auto& value : latest.players)
			if (value.id == id && !value.local && value.position)
			{
				auto destination = *value.position;
				destination.x += 2.f;
				unity::teleport_to_world_point(destination);
				return true;
			}
		return false;
	}
}
