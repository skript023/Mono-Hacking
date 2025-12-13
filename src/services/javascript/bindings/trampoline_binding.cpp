#include "trampoline_binding.hpp"
#include "hooking/detour_hook.hpp"

namespace js::trampoline
{
	using namespace big;

	using detour_factory_t = std::function<detour_hook*(JSContext*, const char* name, void* addr, JSValue cb)>;

	static std::unordered_map<std::string, detour_hook*> g_hooks;

	static std::unordered_map<std::string, detour_factory_t> g_factories;

	enum class RetType
	{
		Void,
		Int,
		Float,
		Ptr
	};

	enum class ArgType
	{
		Ptr,
		Int,
		Float
	};

	static RetType parse_ret(const std::string& s)
	{
		if (s == "void")
			return RetType::Void;
		if (s == "int")
			return RetType::Int;
		if (s == "float")
			return RetType::Float;
		if (s == "ptr")
			return RetType::Ptr;
		throw std::runtime_error("invalid return type");
	}

	static ArgType parse_arg(const std::string& s)
	{
		if (s == "ptr")
			return ArgType::Ptr;
		if (s == "int")
			return ArgType::Int;
		if (s == "float")
			return ArgType::Float;
		throw std::runtime_error("invalid arg type");
	}

	static inline double js_value_to_double_checked(JSContext* ctx, JSValueConst v)
	{
		double d = 0.0;
		if (JS_ToFloat64(ctx, &d, v) < 0)
		{
			// not a number convertible
			return 0.0;
		}
		return d;
	}

	template<typename T>
	T value_to_ptr(JSContext* ctx, JSValueConst v)
	{
		double d = js_value_to_double_checked(ctx, v);
		return reinterpret_cast<T>(static_cast<uintptr_t>((uint64_t)d));
	}

	uint32_t get_array_length(JSContext* ctx, JSValueConst arr)
	{
		JSValue len_val = JS_GetPropertyStr(ctx, arr, "length");
		uint32_t len = 0;
		JS_ToUint32(ctx, &len, len_val);
		JS_FreeValue(ctx, len_val);
		return len;
	}

	static void pack(JSContext* ctx, JSValue& v, void* p)
	{
		v = JS_NewBigUint64(ctx, (uint64_t)p);
	}

	static void pack(JSContext* ctx, JSValue& v, int x)
	{
		v = JS_NewInt32(ctx, x);
	}

	static void pack(JSContext* ctx, JSValue& v, float f)
	{
		v = JS_NewFloat64(ctx, f);
	}

	template<typename T>
	static void pack(JSContext* ctx, JSValue& v, T x)
	{
		static_assert(sizeof(T) <= 8);
		v = JS_NewUint64(ctx, (uint64_t)x);
	}

	template<typename... Args>
	void pack_args(JSContext* ctx, JSValue* argv, Args... args)
	{
		int i = 0;
		((pack(ctx, argv[i++], args)), ...);
	}

	static void unpack_return(JSContext* ctx, JSValue v, int& out)
	{
		JS_ToInt32(ctx, &out, v);
	}

	static void unpack_return(JSContext* ctx, JSValue v, float& out)
	{
		if (!JS_IsNumber(v))
		{
			LOG(WARNING) << "JS return value is not a number";
			out = 0.f; // fallback
			return;
		}

		double d;
		JS_ToFloat64(ctx, &d, v);
		out = (float)d;
	}

	static void unpack_return(JSContext* ctx, JSValue v, bool& out)
	{
		bool d = JS_ToBool(ctx, v);
		
		out = (bool)d;
	}

	static void unpack_return(JSContext* ctx, JSValue v, void*& out)
	{
		out = value_to_ptr<void*>(ctx, v);
	}

	template<typename... Args>
	static void free_args(JSContext* ctx, JSValue* argv)
	{
		for (size_t i = 0; i < sizeof...(Args); ++i)
		{
			JS_FreeValue(ctx, argv[i]);
		}
	}


	template<typename Ret, typename... Args>
	struct JsDetour
	{
		using Fn = Ret(__fastcall*)(Args...);

		inline static Fn original = nullptr;
		inline static JSContext* ctx = nullptr;
		inline static JSValue js_func = JS_UNDEFINED;

		static JsDetour& instance()
		{
			static JsDetour detour;
			return detour;
		}

		static Ret __fastcall trampoline(Args... args)
		{
			if (JS_IsFunction(ctx, js_func))
			{
				JSValue argv[sizeof...(Args)];
				pack_args(ctx, argv, args...);

				JSValue ret = JS_Call(
				    ctx,
				    js_func,
				    JS_UNDEFINED,
				    sizeof...(Args),
				    argv);

				free_args(ctx, argv);

				if (JS_IsException(ret))
				{
					JSValue exc = JS_GetException(ctx);
					JS_FreeValue(ctx, ret);
					LOG(WARNING) << "JS exception occurred";
					return original(args...);
				}
				else
				{
					if constexpr (std::is_same_v<Ret, float>)
					{
						float r{};
						LOG(VERBOSE) << "Detour JS function called successfully, unpacking return value...";
						unpack_return(ctx, ret, r);
						LOG(VERBOSE) << "Detour returned value: " << r;
						JS_FreeValue(ctx, ret);
						return static_cast<float>(r);
					}
					if constexpr (std::is_same_v<Ret, int>)
					{
						int r{};
						LOG(VERBOSE) << "Detour JS function called successfully, unpacking return value...";
						unpack_return(ctx, ret, r);
						JS_FreeValue(ctx, ret);
						LOG(VERBOSE) << "Detour returned value: " << r;
						return r;
					}
					if constexpr (std::is_same_v<Ret, bool>)
					{
						bool r{};
						unpack_return(ctx, ret, r);
						JS_FreeValue(ctx, ret);
						LOG(VERBOSE) << "Detour returned value: " << r;
						return r;
					}
					if constexpr (std::is_same_v<Ret, void*>)
					{
						void* r{};
						unpack_return(ctx, ret, r);
						JS_FreeValue(ctx, ret);
						LOG(VERBOSE) << "Detour returned value: " << r;
						return r;
					}
				}
			}

			if constexpr (!std::is_void_v<Ret>)
				return original(args...);
			else
				original(args...);
		}
	};

	static void register_factories()
	{
		g_factories["float(ptr)"] = [](JSContext* ctx, const char* name, void* addr, JSValue cb) {
			auto& det = JsDetour<float, void*>::instance(); // example

			det.ctx = ctx;
			det.js_func = JS_DupValue(ctx, cb);

			auto* hook = new detour_hook(
			    name,
			    addr,
			    &JsDetour<void, void*>::trampoline);

			det.original = (decltype(det.original))hook->get_original_ptr();
			return hook;
		};

		g_factories["int(ptr)"] = [](JSContext* ctx, const char* name, void* addr, JSValue cb) {
			auto& det = JsDetour<int, void*>::instance(); // example

			det.ctx = ctx;
			det.js_func = JS_DupValue(ctx, cb);

			auto* hook = new detour_hook(
			    name,
			    addr,
			    &JsDetour<void, void*>::trampoline);

			det.original = (decltype(det.original))hook->get_original_ptr();
			return hook;
		};

		g_factories["bool(ptr)"] = [](JSContext* ctx, const char* name, void* addr, JSValue cb) {
			auto& det = JsDetour<bool, void*>::instance(); // example

			det.ctx = ctx;
			det.js_func = JS_DupValue(ctx, cb);

			auto* hook = new detour_hook(
			    name,
			    addr,
			    &JsDetour<void, void*>::trampoline);

			det.original = (decltype(det.original))hook->get_original_ptr();
			return hook;
		};

		g_factories["void(ptr)"] = [](JSContext* ctx, const char* name, void* addr, JSValue cb) {
			auto& det = JsDetour<void, void*>::instance(); // example

			det.ctx = ctx;
			det.js_func = JS_DupValue(ctx, cb);

			auto* hook = new detour_hook(
			    name,
			    addr,
			    &JsDetour<void, void*>::trampoline);

			det.original = (decltype(det.original))hook->get_original_ptr();
			return hook;
		};
	}

	static JSValue js_add_detour(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		// add(name, sigObj, cb)
		if (argc != 3)
			return JS_ThrowTypeError(ctx, "add(name, sig, cb)");

		const char* name = JS_ToCString(ctx, argv[0]);
		JSValue sig = argv[1];
		JSValue cb = argv[2];

		if (!JS_IsObject(sig))
			return JS_ThrowTypeError(ctx, "sig must be object");

		JSValue v_addr = JS_GetPropertyStr(ctx, sig, "addr");
		JSValue v_ret = JS_GetPropertyStr(ctx, sig, "ret");
		JSValue v_args = JS_GetPropertyStr(ctx, sig, "args");

		void* addr = value_to_ptr<void*>(ctx, v_addr);

		std::string ret = JS_ToCString(ctx, v_ret);

		// build signature string: "float(ptr)"
		std::string sig_str = ret + "(";
		
		uint32_t len = get_array_length(ctx, v_args);

		for (uint32_t i = 0; i < len; i++)
		{
			JSValue v = JS_GetPropertyUint32(ctx, v_args, i);
			sig_str += JS_ToCString(ctx, v);
			if (i + 1 < len)
				sig_str += ",";
			JS_FreeValue(ctx, v);
		}
		sig_str += ")";

		LOG(VERBOSE) << "Adding detour '" << name << "' with signature '" << sig_str << "' at address " << addr;

		auto it = g_factories.find(sig_str);
		if (it == g_factories.end())
			return JS_ThrowTypeError(ctx, "unsupported signature");

		auto* hook = it->second(ctx, name, addr, cb);
		g_hooks.emplace(name, hook);

		return JS_UNDEFINED;
	}

	static JSValue js_hook_enable(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const char* name = JS_ToCString(ctx, argv[0]);
		auto it = g_hooks.find(name);

		if (it == g_hooks.end())
			return JS_ThrowReferenceError(ctx, "hook not found");

		it->second->enable_immediately();

		JS_FreeCString(ctx, name);
		return JS_UNDEFINED;
	}

	static JSValue js_hook_disable(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const char* name = JS_ToCString(ctx, argv[0]);
		auto it = g_hooks.find(name);

		if (it == g_hooks.end())
			return JS_ThrowReferenceError(ctx, "hook not found");

		it->second->disable_immediately();

		JS_FreeCString(ctx, name);
		return JS_UNDEFINED;
	}

	static int js_detour_module_init(JSContext* ctx, JSModuleDef* m)
	{
		JS_SetModuleExport(ctx, m, "add", JS_NewCFunction(ctx, js_add_detour, "add", 3));
		JS_SetModuleExport(ctx, m, "enable", JS_NewCFunction(ctx, js_hook_enable, "enable", 1));
		JS_SetModuleExport(ctx, m, "disable", JS_NewCFunction(ctx, js_hook_disable, "disable", 1));

		return 0; // WAJIB
	}

	static JSValue js_call_original(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		// callOriginal(name, args...)
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "call_original(name, ...args)");

		const char* name = JS_ToCString(ctx, argv[0]);
		auto it = g_hooks.find(name);
		if (it == g_hooks.end())
			return JS_ThrowReferenceError(ctx, "hook not found");

		//it->second->get_original<decltype(&JsDetour<void, void*>::trampoline)>()();

		JS_FreeCString(ctx, name);
	}

	static JSModuleDef* js_detour_init(JSContext* ctx, const char* module_name)
	{
		JSModuleDef* m = JS_NewCModule(ctx, module_name, js_detour_module_init);

		if (!m)
			return nullptr;

		JS_AddModuleExport(ctx, m, "add");
		JS_AddModuleExport(ctx, m, "enable");
		JS_AddModuleExport(ctx, m, "disable");

		return m;
	}

	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;

		JSModuleDef* m = js_detour_init(ctx, "detour");
		register_factories();

		if (!m)
			LOG(WARNING) << "Failed to init detour module";
	}
} // namespace js::trampoline