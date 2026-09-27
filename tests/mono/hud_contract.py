"""Check HUD metadata and reference writes against the installed Unity Mono runtime.

Runs in a separate process; does not attach to or modify the running game.
Usage: python tests/mono/hud_contract.py <Valheim directory>
"""
import ctypes as c
import os
from pathlib import Path
import sys

game = Path(sys.argv[1]).resolve()
managed = game / "valheim_Data/Managed"
runtime = game / "MonoBleedingEdge/EmbedRuntime"
dll_directory = os.add_dll_directory(str(runtime))
mono = c.CDLL(str(runtime / "mono-2.0-bdwgc.dll"))
P = c.c_void_p


def api(name, result, *args):
    function = getattr(mono, name)
    function.restype = result
    function.argtypes = args
    return function


api("mono_set_assemblies_path", None, c.c_char_p)(os.fsencode(managed))
domain = api("mono_jit_init_version", P, c.c_char_p, c.c_char_p)(b"hud-contract", b"v4.0.30319")
assert domain, "Mono initialization failed"
assembly_open = api("mono_domain_assembly_open", P, P, c.c_char_p)
get_image = api("mono_assembly_get_image", P, P)
get_class = api("mono_class_from_name", P, P, c.c_char_p, c.c_char_p)
get_field = api("mono_class_get_field_from_name", P, P, c.c_char_p)
get_methods = api("mono_class_get_methods", P, P, c.POINTER(P))
method_name = api("mono_method_get_name", c.c_char_p, P)
signature = api("mono_method_signature", P, P)
parameter_count = api("mono_signature_get_param_count", c.c_uint, P)
parameter = api("mono_signature_get_params", P, P, c.POINTER(P))
type_name = api("mono_type_get_name", P, P)
free = api("mono_free", None, P)


def image(name):
    assembly = assembly_open(domain, os.fsencode(managed / name))
    assert assembly, name
    return get_image(assembly)


def exact(klass, name, expected):
    iterator = P()
    while method := get_methods(klass, c.byref(iterator)):
        if method_name(method) != name:
            continue
        sig = signature(method)
        if parameter_count(sig) != len(expected):
            continue
        actual = []
        params = P()
        for _ in expected:
            allocated = type_name(parameter(sig, c.byref(params)))
            actual.append(c.string_at(allocated).decode())
            free(allocated)
        if actual == expected:
            return method
    raise AssertionError((name, expected))


core = image("UnityEngine.CoreModule.dll")
unity_object = get_class(core, b"UnityEngine", b"Object")
exact(unity_object, b"Instantiate", ["UnityEngine.Object", "UnityEngine.Transform", "System.Boolean"])
exact(unity_object, b"Destroy", ["UnityEngine.Object"])
hud = get_class(image("assembly_valheim.dll"), b"", b"Hud")
exact(hud, b"get_instance", [])
for name in [b"m_foodBars", b"m_foodIcons", b"m_foodTime"]:
    assert get_field(hud, name), name
print("PASS: installed HUD fields and exact Instantiate overload")

corlib = api("mono_get_corlib", P)()
object_class = get_class(corlib, b"System", b"Object")
holder_class = get_class(corlib, b"System.Runtime.Remoting.Messaging", b"Header")
assert holder_class
holder = api("mono_object_new", P, P, P)(domain, holder_class)
value_field = get_field(holder_class, b"Value")
assert value_field
retain = api("mono_gchandle_new_v2", c.c_size_t, P, c.c_int)
target = api("mono_gchandle_get_target_v2", P, c.c_size_t)
release = api("mono_gchandle_free_v2", None, c.c_size_t)
holder_handle = retain(holder, 1)
assert target(holder_handle) == holder
array_new = api("mono_array_new", P, P, P, c.c_size_t)
array_address = api("mono_array_addr_with_size", P, P, c.c_int, c.c_size_t)
array_length = api("mono_array_length", c.c_size_t, P)
set_reference = api("mono_gc_wbarrier_set_arrayref", None, P, P, P)
set_field = api("mono_field_set_value", None, P, P, P)
get_value = api("mono_field_get_value", None, P, P, P)
collect = api("mono_gc_collect", None, c.c_int)
generation = api("mono_gc_max_generation", c.c_int)()
string_new = api("mono_string_new", P, P, c.c_char_p)

for count in [3, 10, 4, 3]:
    array = array_new(domain, object_class, count)
    handle = retain(array, 1)
    for index in range(count):
        value = string_new(domain, f"food-{index}".encode())
        set_reference(array, array_address(array, c.sizeof(P), index), value)
    # Reference fields take the object itself, unlike value fields and get_field_value.
    set_field(target(holder_handle), value_field, array)
    release(handle)
    collect(generation)
    actual = P()
    get_value(target(holder_handle), value_field, c.byref(actual))
    assert actual.value == array
    assert array_length(actual) == count
    for index in range(count):
        assert P.from_address(array_address(actual, c.sizeof(P), index)).value
set_field(target(holder_handle), value_field, None)
actual = P(1)
get_value(target(holder_handle), value_field, c.byref(actual))
assert actual.value is None
release(holder_handle)
print("PASS: reference field writes, array write barriers, GC retention, grow/shrink, null")

# The custom EatFood path writes both reference and scalar fields on Player.Food.
game_image = image("assembly_valheim.dll")
food_class = get_class(game_image, b"", b"Player/Food")
item_class = get_class(game_image, b"", b"ItemDrop/ItemData")
assert food_class and item_class
object_new = api("mono_object_new", P, P, P)
food_handle = retain(object_new(domain, food_class), 1)
item_handle = retain(object_new(domain, item_class), 1)
food = target(food_handle)
item_field = get_field(food_class, b"m_item")
name_field = get_field(food_class, b"m_name")
time_field = get_field(food_class, b"m_time")
assert item_field and name_field and time_field
set_field(food, item_field, target(item_handle))
set_field(food, name_field, string_new(domain, b"CookedMeat"))
seconds = c.c_float(120)
set_field(food, time_field, c.byref(seconds))
collect(generation)
actual_item = P()
actual_name = P()
actual_time = c.c_float()
get_value(food, item_field, c.byref(actual_item))
get_value(food, name_field, c.byref(actual_name))
get_value(food, time_field, c.byref(actual_time))
assert actual_item.value == target(item_handle)
assert actual_name.value and actual_time.value == 120
release(item_handle)
release(food_handle)
print("PASS: real Player.Food item/name references and scalar time survive GC")

seman_class = get_class(game_image, b"", b"SEMan")
assert seman_class
get_effect = exact(seman_class, b"GetStatusEffect", ["System.Int32"])
legacy_lookup = api("mono_class_get_method_from_name", P, P, c.c_char_p, c.c_int)
legacy_method = legacy_lookup(seman_class, b"GetStatusEffect", 1)
params = P()
legacy_type = type_name(parameter(signature(legacy_method), c.byref(params)))
legacy_parameter = c.string_at(legacy_type).decode()
free(legacy_type)
print("Legacy GetStatusEffect(name, count) resolves:", legacy_parameter, flush=True)
for first in ["System.Int32", "StatusEffect"]:
    exact(seman_class, b"AddStatusEffect", [first, "System.Boolean", "System.Int32", "System.Single", "System.Int16"])
exact(seman_class, b"RemoveStatusEffect", ["System.Int32", "System.Boolean"])
seman_handle = retain(object_new(domain, seman_class), 1)
zero_hash = c.c_int(0)
arguments = (P * 1)(c.cast(c.byref(zero_hash), P))
exception = P()
invoke = api("mono_runtime_invoke", P, P, P, c.POINTER(P), c.POINTER(P))
assert invoke(get_effect, target(seman_handle), arguments, c.byref(exception)) is None
assert exception.value is None
release(seman_handle)
print("PASS: exact status effect overloads and missing-effect query")
