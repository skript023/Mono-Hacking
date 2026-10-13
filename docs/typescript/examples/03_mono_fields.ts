// API: get_field_value, set_field_int, set_field_float, set_field_string.
// This example deliberately defines write helpers without calling them at
// load time. Call them only with a verified live object and matching field.

const klass = mono.get_class("Player", "assembly_valheim");
const player = unity.get_local_player();

if (klass !== 0 && player !== 0) {
  const healthField = mono.get_field(klass, "m_baseHP");
  if (healthField !== 0) {
    // Raw native representation only: this is not a typed C# field reader.
    console.log("Raw m_baseHP field value:", mono.get_field_value(player, healthField));
  }
}

function writeIntField(objectPointer: number, classPointer: number, name: string, value: number): void {
  const field = mono.get_field(classPointer, name);
  if (objectPointer !== 0 && field !== 0) mono.set_field_int(objectPointer, field, value);
}

function writeFloatField(objectPointer: number, classPointer: number, name: string, value: number): void {
  const field = mono.get_field(classPointer, name);
  if (objectPointer !== 0 && field !== 0) mono.set_field_float(objectPointer, field, value);
}

function writeStringField(objectPointer: number, classPointer: number, name: string, value: string): void {
  const field = mono.get_field(classPointer, name);
  if (objectPointer !== 0 && field !== 0) mono.set_field_string(objectPointer, field, value);
}

// Example calls (uncomment only after verifying the target field and type):
// writeIntField(player, klass, "someIntField", 10);
// writeFloatField(player, klass, "someFloatField", 10.5);
// writeStringField(player, klass, "someStringField", "example");

