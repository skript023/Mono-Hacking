#include "command_binding.hpp"
#include "commands/looped_command.hpp"

namespace js::command
{
	using namespace big;
	// ... class IDs tetap sama
	JSClassID bool_command_class_id;
	JSClassID looped_command_class_id;

	inline void run_pending(JSRuntime* rt)
	{
		JSContext* pctx = nullptr;
		while (JS_ExecutePendingJob(rt, &pctx))
		{
			// loop sampai semua job pending selesai
		}
	}
	JSValue get_js_method(JSContext* ctx, JSValueConst obj, const char* name)
	{
		JSAtom atom = JS_NewAtom(ctx, name);
		JSValue val = JS_GetProperty(ctx, obj, atom); // resolve prototype chain
		JS_FreeAtom(ctx, atom);
		return val;
	}
	// =========================
	// js_bool_command
	// =========================
	class js_bool_command : public bool_command
	{
	public:
		JSContext* ctx;
		JSValue js_this;

		js_bool_command(JSContext* ctx,
		    const char* name,
		    const char* label,
		    const char* desc,
		    bool def,
		    JSValue js_obj) :
		    bool_command(name, label, desc, def),
		    ctx(ctx),
		    js_this(js_obj)
		{
		}

		~js_bool_command()
		{
			JS_FreeValue(ctx, js_this);
			
			
		}

		void on_enable() override
		{
			auto js_onEnable = JS_GetPropertyStr(ctx, js_this, "onEnable");

			if (!JS_IsUndefined(js_onEnable))
			{
				JSValue ret = JS_Call(ctx, js_onEnable, js_this, 0, nullptr);
				JS_FreeValue(ctx, ret);
			}

			JS_FreeValue(ctx, js_onEnable);
		}

		void on_disable() override
		{
			JSValue js_onDisable = JS_GetPropertyStr(ctx, js_this, "onDisable");

			if (!JS_IsUndefined(js_onDisable))
			{
				JSValue ret = JS_Call(ctx, js_onDisable, js_this, 0, nullptr);
				JS_FreeValue(ctx, ret);
			}

			JS_FreeValue(ctx, js_onDisable);
		}
	};

	// =========================
	// js_looped_command
	// =========================
	class js_looped_command : public looped_command
	{
	public:
		JSContext* ctx;
		JSValue js_this;

		js_looped_command(JSContext* ctx_, const char* name, const char* label, const char* desc, JSValue js_obj) :
		    looped_command(name, label, desc),
		    ctx(ctx_),
		    js_this(JS_DupValue(ctx_, js_obj))
		{
		}

		~js_looped_command()
		{
			JS_FreeValue(ctx, js_this);
		}

		void on_tick() override
		{
			JSValue js_onTick = get_js_method(ctx, js_this, "onTick");
#ifdef _DEBUG
			LOG(VERBOSE) << "onTick is function?: " << JS_IsFunction(ctx, js_onTick);
#endif

			if (JS_IsFunction(ctx, js_onTick))
			{
				JSValue ret = JS_Call(ctx, js_onTick, js_this, 0, nullptr);
				JS_FreeValue(ctx, ret);
			}
			JS_FreeValue(ctx, js_onTick);
		}

		void on_disable() override
		{
			JSValue js_onDisable = JS_GetPropertyStr(ctx, js_this, "onDisable");
			if (JS_IsFunction(ctx, js_onDisable))
			{
				JSValue ret = JS_Call(ctx, js_onDisable, js_this, 0, nullptr);
				JS_FreeValue(ctx, ret);
			}
			JS_FreeValue(ctx, js_onDisable);
		}
	};

	// =========================
	// JS binding functions
	// =========================
	static JSValue js_get_state(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);
		if (!native)
			return JS_ThrowTypeError(ctx, "get_state invalid");
		return JS_NewBool(ctx, native->get_state());
	}

	static JSValue js_set_state(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);
		if (!native)
			return JS_ThrowTypeError(ctx, "set_state invalid");
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "set_state missing arg");

		bool state = JS_ToBool(ctx, argv[0]);
		native->set_state(state);
		return JS_UNDEFINED;
	}

	// =========================
	// Register classes
	// =========================
	void register_bool_command(JSContext* ctx, JSValue global)
	{
		JS_NewClassID(&bool_command_class_id);

		JSClassDef def{};
		def.class_name = "BoolCommand";
		def.finalizer = [](JSRuntime* rt, JSValue val) {
			js_bool_command* native = (js_bool_command*)JS_GetOpaque(val, bool_command_class_id);
			delete native;
		};

		JS_NewClass(JS_GetRuntime(ctx), bool_command_class_id, &def);

		JSValue proto = JS_NewObject(ctx);
		JS_SetPropertyStr(ctx, proto, "get_state", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
			js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, bool_command_class_id);
			if (!native)
				return JS_ThrowTypeError(ctx, "get_state invalid");
			return JS_NewBool(ctx, native->get_state());
		}, "get_state", 0));

		JS_SetPropertyStr(ctx, proto, "set_state", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
			js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);
			if (!native)
				return JS_ThrowTypeError(ctx, "set_state invalid");
			if (argc < 1)
				return JS_ThrowTypeError(ctx, "set_state missing arg");

			bool state = JS_ToBool(ctx, argv[0]);
			native->set_state(state);
			return JS_UNDEFINED;
		}, "set_state", 1));

		JSValue ctor = JS_NewCFunction2(ctx, [](JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) -> JSValue {
			if (argc < 3)
				return JS_ThrowTypeError(ctx, "new BoolCommand(name,label,desc)");
			const char* name = JS_ToCString(ctx, argv[0]);
			const char* label = JS_ToCString(ctx, argv[1]);
			const char* desc = JS_ToCString(ctx, argv[2]);
			bool def = (argc >= 4) ? JS_ToBool(ctx, argv[3]) : false;

			JSValue obj = JS_NewObjectProtoClass(ctx, JS_GetPrototype(ctx, new_target), bool_command_class_id);
			js_bool_command native = js_bool_command(ctx, name, label, desc, def, obj);
			JS_SetOpaque(obj, &native);

			JS_SetPropertyStr(ctx, obj, "get_state", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
				js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, bool_command_class_id);
				if (!native)
					return JS_ThrowTypeError(ctx, "get_state invalid");
				return JS_NewBool(ctx, native->get_state());
			}, "get_state", 0));

			JS_SetPropertyStr(ctx, obj, "set_state", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
				js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);
				if (!native)
					return JS_ThrowTypeError(ctx, "set_state invalid");
				if (argc < 1)
					return JS_ThrowTypeError(ctx, "set_state missing arg");

				bool state = JS_ToBool(ctx, argv[0]);
				native->set_state(state);
				return JS_UNDEFINED;
			}, "set_state", 1));

			JS_FreeCString(ctx, name);
			JS_FreeCString(ctx, label);
			JS_FreeCString(ctx, desc);
			return obj;
		},
		    "BoolCommand",
		    3,
		    JS_CFUNC_constructor,
		    0);

		JS_SetConstructor(ctx, ctor, proto);
		JS_SetClassProto(ctx, bool_command_class_id, proto);
		JS_SetPropertyStr(ctx, global, "BoolCommand", ctor);
	}

	void register_looped_command(JSContext* ctx, JSValue global)
	{
		JS_NewClassID(&looped_command_class_id);

		JSClassDef def{};
		def.class_name = "LoopedCommand";
		def.finalizer = [](JSRuntime* rt, JSValue val) {
			js_looped_command* native = (js_looped_command*)JS_GetOpaque(val, looped_command_class_id);
			delete native;
		};
		JS_NewClass(JS_GetRuntime(ctx), looped_command_class_id, &def);

		JSValue proto = JS_NewObject(ctx);
		JS_SetPropertyStr(ctx, proto, "tick", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
			js_looped_command* native = (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);
			if (!native)
				return JS_ThrowTypeError(ctx, "tick invalid");
			native->tick();
			return JS_UNDEFINED;
		}, "tick", 0));

		JS_SetPropertyStr(ctx, proto, "onTick", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
			// dummy, do nothing
			return JS_UNDEFINED;
		}, "onTick", 0));

		JS_SetPropertyStr(ctx, proto, "set_state", JS_NewCFunction(ctx, js_set_state, "set_state", 1));
		JS_SetPropertyStr(ctx, proto, "get_state", JS_NewCFunction(ctx, js_get_state, "get_state", 0));

		JSValue ctor = JS_NewCFunction2(ctx, [](JSContext* ctx, JSValueConst new_target, int argc, JSValueConst* argv) -> JSValue {
			if (argc < 3)
				return JS_ThrowTypeError(ctx, "new LoopedCommand(name,label,desc)");
			const char* name = JS_ToCString(ctx, argv[0]);
			const char* label = JS_ToCString(ctx, argv[1]);
			const char* desc = JS_ToCString(ctx, argv[2]);

			JSValue obj = JS_NewObjectProtoClass(ctx, JS_GetPrototype(ctx, new_target), looped_command_class_id);
			js_looped_command native = js_looped_command(ctx, name, label, desc, obj);
			JS_SetOpaque(obj, &native);

			JS_SetPropertyStr(ctx, obj, "set_state", JS_NewCFunction(ctx, js_set_state, "set_state", 1));
			JS_SetPropertyStr(ctx, obj, "get_state", JS_NewCFunction(ctx, js_get_state, "get_state", 0));

			JS_SetPropertyStr(ctx, obj, "onTick", JS_NewCFunction(ctx, [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
				// dummy, do nothing
				return JS_UNDEFINED;
			}, "onTick", 0));

			JSValue proto = JS_GetPrototype(ctx, obj);
			JSValue proto_onTick = JS_GetPropertyStr(ctx, proto, "onTick");
			if (JS_IsFunction(ctx, proto_onTick))
			{
				// buat function bound
				JSValue bound = JS_Call(ctx, proto_onTick, obj, 0, nullptr);
				JS_SetPropertyStr(ctx, obj, "onTick", proto_onTick); // set ke instance
			}
			JS_FreeValue(ctx, proto_onTick);

			JS_FreeCString(ctx, name);
			JS_FreeCString(ctx, label);
			JS_FreeCString(ctx, desc);
			return obj;
		},
		    "LoopedCommand",
		    3,
		    JS_CFUNC_constructor,
		    0);

		JS_SetConstructor(ctx, ctor, proto);
		JS_SetClassProto(ctx, looped_command_class_id, proto);
		JS_SetPropertyStr(ctx, global, "LoopedCommand", ctor);
	}

	void bind(qjs::Context& ctx)
	{
		auto global = ctx.global();
		register_bool_command(ctx.ctx, global.v);
		register_looped_command(ctx.ctx, global.v);
	}
}
