// API: get_root_domain, get_class, get_method, get_field,
// get_class_from_method, get_compile_method, get_vtable,
// get_static_field_data, get_static_field_data_vtable,
// get_field_offset, get_static_field_value.
import { Mono, mono as monoModule } from "mono";

const domain = mono.get_root_domain();
const klass = mono.get_class("Player", "assembly_valheim");

console.log("Root domain:", domain);
console.log("Player class:", klass);
console.log("Module export agrees:", monoModule.get_root_domain() === domain);
console.log("Class-style export:", Mono.get_class("Player", "assembly_valheim"));

if (klass === 0) {
  console.warn("Player class was not found; check class and assembly names.");
} else {
  const method = mono.get_method("Player", "GetCurrentBiome", 0, "assembly_valheim");
  const field = mono.get_field(klass, "m_baseHP");
  const vtable = mono.get_vtable(klass);
  console.log("Method / field / vtable:", method, field, vtable);

  if (method !== 0) {
    console.log("Method owner class:", mono.get_class_from_method(method));
  }
  if (field !== 0) {
    console.log("Field offset:", mono.get_field_offset(field));
  }
  if (vtable !== 0) {
    console.log("Static data from vtable:", mono.get_static_field_data_vtable(vtable));
  }
  console.log("Static data for class:", mono.get_static_field_data(klass));
}

const compiledAddress = mono.get_compile_method("Player", "GetCurrentBiome", 0, "assembly_valheim");
console.log("Compiled method address:", compiledAddress);

// This helper searches Assembly-CSharp (the binding's fixed default), not
// assembly_valheim, and returns the static field slot address rather than value.
function findStaticFieldAddress(className: string, fieldName: string): number {
  return mono.get_static_field_value(className, fieldName);
}
