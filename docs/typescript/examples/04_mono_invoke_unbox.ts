// API: invoke_method, unbox, unbox_vec3.
// Verify the target method's exact signature and argument types in the game
// assembly. This example only invokes a zero-argument getter.

const player = unity.get_local_player();
const method = mono.get_method("Player", "GetCurrentBiome", 0, "assembly_valheim");

if (player !== 0 && method !== 0) {
  const boxedResult = mono.invoke_method(method, player);
  console.log("Invocation result object:", boxedResult);

  if (boxedResult !== 0) {
    console.log("Unboxed data address:", mono.unbox(boxedResult));
  }
}

function readBoxedVector3(boxedVector3: number): void {
  // Call only when boxedVector3 is known to contain a boxed Unity Vector3.
  const vector = mono.unbox_vec3(boxedVector3);
  if (vector !== null) console.log("Unboxed Vector3:", vector.x, vector.y, vector.z);
}

// For a method with arguments, pass primitive values or strings after the
// instance pointer, for example: mono.invoke_method(method, player, 1, "text").
// The binding does not marshal arbitrary C# structs, arrays, enums, ref/out, etc.
