#include "item_icons.hpp"
#include "fiber_pool.hpp"
#include "mono/mono.hpp"
#include "renderer.hpp"
#include "utility/unity.hpp"

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace big
{
	namespace
	{
		MonoMethod* method(const char* klass, const char* name, int count, bool engine = true)
		{
			auto result = mono::get_method(klass, name, count, engine ? "UnityEngine.CoreModule" : "assembly_valheim", engine ? "UnityEngine" : "");
			if (!result)
				throw std::runtime_error(std::format("Missing {}.{}", klass, name));
			return result;
		}

		MonoObject* call(MonoMethod* method, MonoObject* instance = nullptr, void** args = nullptr)
		{
			MonoObject* exception = nullptr;
			auto result = mono::invoke_method(method, instance, args, &exception);
			if (exception)
				throw std::runtime_error("Unity rejected icon readback");
			return result;
		}

		template<typename T>
		T value(MonoObject* object)
		{
			if (!object)
				throw std::runtime_error("Missing icon property");
			return *static_cast<T*>(mono::object_unbox(object));
		}

		struct rect
		{
			float x, y, width, height;
		};
		struct vector2
		{
			float x, y;
		};

		struct readback_scope
		{
			MonoMethod* set_active = method("RenderTexture", "set_active", 1);
			MonoMethod* release = method("RenderTexture", "ReleaseTemporary", 1);
			MonoMethod* destroy = method("Object", "Destroy", 1);
			MonoObject* previous = call(method("RenderTexture", "get_active", 0));
			MonoObject* temporary = nullptr;
			MonoObject* readable = nullptr;
			~readback_scope()
			{
				void* restore[] = {previous};
				mono::invoke_method(set_active, nullptr, restore);
				if (temporary)
				{
					void* args[] = {temporary};
					mono::invoke_method(release, nullptr, args);
				}
				if (readable)
				{
					void* args[] = {readable};
					mono::invoke_method(destroy, nullptr, args);
				}
			}
		};

	}

	std::vector<unsigned char> item_icons::read_icon_impl(const std::string& prefab_name)
	{
		auto database = unity::get_object_db();
		if (!database)
			return {};
		auto get_prefab = mono::get_method_overload("ObjectDB", "GetItemPrefab", 1, nullptr, "System.String", "assembly_valheim");
		if (!get_prefab)
			return {};
		void* prefab_args[] = {mono::to_mono_string(prefab_name)};
		auto prefab = call(get_prefab, database, prefab_args);
		if (!prefab)
			return {};
		auto component_method = mono::get_method_overload("GameObject", "GetComponent", 1, nullptr, "System.Type", "UnityEngine.CoreModule", "UnityEngine");
		auto drop_class = mono::get_class("ItemDrop", "assembly_valheim");
		if (!component_method || !drop_class)
			return {};
		void* component_args[] = {mono::reflection_type(drop_class)};
		auto drop = call(component_method, prefab, component_args);
		MonoObject* data = nullptr;
		mono::get_field_value(drop, mono::get_field(drop_class, "m_itemData"), &data);
		if (!data)
			return {};
		auto sprite = call(method("ItemDrop/ItemData", "GetIcon", 0, false), data);
		if (!sprite)
			return {};
		auto texture = call(method("Sprite", "get_texture", 0), sprite);
		if (!texture)
			return {};
		auto bounds = value<rect>(call(method("Sprite", "get_textureRect", 0), sprite));
		int width = value<int>(call(method("Texture", "get_width", 0), texture));
		int height = value<int>(call(method("Texture", "get_height", 0), texture));
		if (width <= 0 || height <= 0 || bounds.width <= 0 || bounds.height <= 0)
			return {};

		// Blit4 is the Vector2 scale/offset overload, not the Material overload.
		auto blit = method("Graphics", "Blit4", 4);
		readback_scope resources;
		int size = icon_size, depth = 0;
		void* temporary_args[] = {&size, &size, &depth};
		resources.temporary = call(method("RenderTexture", "GetTemporary", 3), nullptr, temporary_args);
		if (!resources.temporary)
			return {};
		vector2 scale{bounds.width / width, bounds.height / height};
		vector2 offset{bounds.x / width, bounds.y / height};
		void* blit_args[] = {texture, resources.temporary, &scale, &offset};
		call(blit, nullptr, blit_args);
		void* active_args[] = {resources.temporary};
		call(resources.set_active, nullptr, active_args);
		auto texture_class = mono::get_class("Texture2D", "UnityEngine.CoreModule", "UnityEngine");
		if (!texture_class)
			return {};
		resources.readable = mono::object_new(texture_class);
		if (!resources.readable)
			return {};
		void* ctor_args[] = {&size, &size};
		call(method("Texture2D", ".ctor", 2), resources.readable, ctor_args);
		rect region{0, 0, float(size), float(size)};
		int origin = 0;
		bool recalculate = false;
		void* read_args[] = {&region, &origin, &origin, &recalculate};
		call(method("Texture2D", "ReadPixels", 4), resources.readable, read_args);
		auto pixels = reinterpret_cast<MonoArray*>(call(method("Texture2D", "GetPixels32", 0), resources.readable));
		if (!pixels || mono::array_length(pixels) != size * size)
			return {};
		auto source = static_cast<const unsigned char*>(mono::array_with_size(pixels, 4, 0));
		std::vector<unsigned char> result(size * size * 4);
		for (int y = 0; y < size; ++y)
			std::memcpy(result.data() + y * size * 4, source + (size - 1 - y) * size * 4, size * 4);
		return result;
	}
	void item_icons::invalidate_impl()
	{
		std::lock_guard lock(cache_mutex);
		++revision;
		reset_requested = true;
	}

	void item_icons::begin_frame_impl()
	{
		if (!g_renderer)
			return;
		std::lock_guard lock(cache_mutex);
		if (renderer_generation != g_renderer->texture_generation())
		{
			renderer_generation = g_renderer->texture_generation();
			for (auto& [name, icon] : cache)
				icon.texture = 0;
		}
		if (reset_requested)
		{
			for (const auto& [name, icon] : cache)
				g_renderer->release_texture(icon.texture);
			cache.clear();
			reset_requested = false;
		}
		if (!job_pending && g_fiber_pool && g_running && std::any_of(cache.begin(), cache.end(), [](const auto& item) {
			    return item.second.status == state::queued;
		    }))
		{
			g_fiber_pool->queue_job([this] {
				load_next_impl();
			});
			job_pending = true;
		}
	}

	ImTextureID item_icons::get_impl(const std::string& prefab)
	{
		if (!g_renderer || !g_renderer->m_init)
			return 0;
		std::lock_guard lock(cache_mutex);
		const int frame = ImGui::GetFrameCount();
		if (!cache.contains(prefab))
		{
			if (cache.size() >= cache_limit)
			{
				auto oldest = std::min_element(cache.begin(), cache.end(), [](const auto& a, const auto& b) {
					return a.second.last_frame < b.second.last_frame;
				});
				if (oldest->second.last_frame == frame)
					return 0;
				g_renderer->release_texture(oldest->second.texture);
				cache.erase(oldest);
			}
			auto& added = cache[prefab];
			added.ticket = ++next_ticket;
		}
		auto& icon = cache.at(prefab);
		icon.last_frame = frame;
		if (icon.status == state::failed && clock::now() >= icon.retry_at)
			icon.status = state::queued;
		if (!icon.texture && icon.status == state::ready && upload_frame != frame && clock::now() >= icon.retry_at)
		{
			upload_frame = frame;
			icon.texture = g_renderer->upload_rgba(icon.pixels.data(), icon_size, icon_size);
			if (!icon.texture)
				icon.retry_at = clock::now() + std::chrono::seconds(5);
		}
		return icon.texture;
	}

	void item_icons::load_next_impl()
	{
		std::string name;
		uint64_t request_revision, ticket;
		{
			std::lock_guard lock(cache_mutex);
			if (reset_requested)
			{
				job_pending = false;
				return;
			}
			auto newest = cache.end();
			for (auto it = cache.begin(); it != cache.end(); ++it)
				if (it->second.status == state::queued && (newest == cache.end() || it->second.last_frame > newest->second.last_frame))
					newest = it;
			if (newest == cache.end())
			{
				job_pending = false;
				return;
			}
			name = newest->first;
			ticket = newest->second.ticket;
			newest->second.status = state::loading;
			request_revision = revision;
		}
		std::vector<unsigned char> pixels;
		try
		{
			pixels = read_icon_impl(name);
		}
		catch (const std::exception& e)
		{
			LOG(WARNING) << "Item icon '" << name << "': " << e.what();
		}
		std::lock_guard lock(cache_mutex);
		job_pending = false;
		auto it = cache.find(name);
		if (request_revision != revision || it == cache.end() || it->second.ticket != ticket)
			return;
		it->second.status = pixels.empty() ? state::failed : state::ready;
		it->second.pixels = std::move(pixels);
		it->second.retry_at = it->second.status == state::failed ? clock::now() + std::chrono::seconds(30) : clock::time_point{};
	}
}
