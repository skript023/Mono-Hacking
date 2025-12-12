#include "command_binding.hpp"
#include "commands/looped_command.hpp"

namespace js::command
{
	using namespace big;

	JSClassID looped_command_class_id;

	class js_looped_command : public looped_command
	{
	public:
		JSContext* ctx;
		JSValue js_this;   // JS object instance
		JSValue js_onTick; // JS function callback
		js_looped_command(JSContext* ctx,
		    const char* name,
		    const char* label,
		    const char* description,
		    JSValue js_obj) :
		    looped_command(name, label, description),
		    ctx(ctx),
		    js_this(js_obj)
		{
			JS_DupValue(ctx, js_this);

			js_onTick = JS_GetPropertyStr(ctx, js_this, "onTick");
			if (!JS_IsFunction(ctx, js_onTick))
			{
				js_onTick = JS_UNDEFINED;
			}
		}

		~js_looped_command()
		{
			JS_FreeValue(ctx, js_this);
			JS_FreeValue(ctx, js_onTick);
		}

		virtual void on_tick() override
		{
			if (JS_IsUndefined(js_onTick))
				return;

			JSValue ret = JS_Call(ctx, js_onTick, js_this, 0, nullptr);
			JS_FreeValue(ctx, ret);
		}
	};

	static JSValue js_looped_command_ctor(JSContext* ctx, JSValueConst new_target,
	    int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "new LoopedCommand(name, label, desc)");

		const char* name = JS_ToCString(ctx, argv[0]);
		const char* label = JS_ToCString(ctx, argv[1]);
		const char* desc = JS_ToCString(ctx, argv[2]);

		JSValue obj = JS_NewObjectClass(ctx, looped_command_class_id);

		js_looped_command* native =
		    new js_looped_command(ctx, name, label, desc, obj);

		JS_SetOpaque(obj, native);

		JS_FreeCString(ctx, name);
		JS_FreeCString(ctx, label);
		JS_FreeCString(ctx, desc);

		return obj;
	}

	static JSValue js_looped_command_tick(JSContext* ctx,
	    JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		js_looped_command* native =
		    (js_looped_command*)JS_GetOpaque(this_val, looped_command_class_id);

		if (!native)
			return JS_ThrowTypeError(ctx, "LoopedCommand.tick called on invalid object");

		native->tick();
		return JS_UNDEFINED;
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
		JS_SetPropertyStr(ctx, proto, "tick", JS_NewCFunction(ctx, js_looped_command_tick, "tick", 0));

		JSValue ctor = JS_NewCFunction2(ctx, js_looped_command_ctor, "LoopedCommand", 3, JS_CFUNC_constructor, 0);

		JS_SetConstructor(ctx, ctor, proto);

		JS_SetPropertyStr(ctx, global, "LoopedCommand", ctor);
	}

	void bind(qjs::Context& ctx)
	{
		register_looped_command(ctx.ctx, ctx.global());
	}
}