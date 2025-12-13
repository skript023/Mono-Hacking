#include "store_binding.hpp"
#include "ui/store_mgr.hpp"

namespace js::storage
{
	using namespace big;

	static JSValue js_store_get_bool(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_bool(key, default?)");

		const char* key = JS_ToCString(ctx, argv[0]);
		bool def = false;

		if (argc >= 2)
			def = JS_ToBool(ctx, argv[1]);

		bool result = store::get_bool(key, def);

		JS_FreeCString(ctx, key);
		return JS_NewBool(ctx, result);
	}

	static JSValue js_store_get_int(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_int(key, default?)");

		const char* key = JS_ToCString(ctx, argv[0]);
		int def = 0;

		if (argc >= 2)
			JS_ToInt32(ctx, &def, argv[1]);

		int result = store::get_int(key, def);

		JS_FreeCString(ctx, key);
		return JS_NewInt32(ctx, result);
	}

	static JSValue js_store_get_float(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_float(key, default?)");

		const char* key = JS_ToCString(ctx, argv[0]);
		double def = 0.0;

		if (argc >= 2)
			JS_ToFloat64(ctx, &def, argv[1]);

		float result = store::get_float(key, (float)def);

		JS_FreeCString(ctx, key);
		return JS_NewFloat64(ctx, result);
	}

	static JSValue js_store_set_bool(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "set_bool(key, value)");

		const char* key = JS_ToCString(ctx, argv[0]);
		bool value = JS_ToBool(ctx, argv[1]);

		store::set_bool(key, value);

		JS_FreeCString(ctx, key);
		return JS_UNDEFINED;
	}

	static JSValue js_store_set_int(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "set_int(key, value)");

		const char* key = JS_ToCString(ctx, argv[0]);
		int value;
		JS_ToInt32(ctx, &value, argv[1]);

		store::set_int(key, value);

		JS_FreeCString(ctx, key);
		return JS_UNDEFINED;
	}

	static JSValue js_store_set_float(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "set_float(key, value)");

		const char* key = JS_ToCString(ctx, argv[0]);
		double value;
		JS_ToFloat64(ctx, &value, argv[1]);

		store::set_float(key, (float)value);

		JS_FreeCString(ctx, key);
		return JS_UNDEFINED;
	}

	static int js_store_module_init(JSContext* ctx, JSModuleDef* m)
	{
		JS_SetModuleExport(ctx, m, "getBool", JS_NewCFunction(ctx, js_store_get_bool, "getBool", 2));

		JS_SetModuleExport(ctx, m, "setBool", JS_NewCFunction(ctx, js_store_set_bool, "setBool", 2));

		return 0;
	}

	static JSModuleDef* js_store_init(JSContext* ctx, const char* name)
	{
		JSModuleDef* m = JS_NewCModule(ctx, name, js_store_module_init);
		if (!m)
			return nullptr;

		JS_AddModuleExport(ctx, m, "getBool");
		JS_AddModuleExport(ctx, m, "setBool");

		return m;
	}

	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;

		js_store_init(ctx, "store");
	}
}