// Compile to JavaScript before placing the output in ./scripts.
// This sample only reads runtime state; it does not write to game objects.

const domain = mono.get_root_domain();
const playerPointer = unity.get_local_player();
const playerClass = mono.get_class("Player", "Assembly-CSharp");

console.log("Mono domain available:", domain !== 0);
console.log("Local player available:", playerPointer !== 0);
console.log("Player class available:", playerClass !== 0);

const samplePosition = new Vector3(12.5, 45, -9.8);
console.log("Vector3 sample:", samplePosition.x, samplePosition.y, samplePosition.z);
console.log("Zone system available:", unity.get_zone_system() !== 0);
console.log("Environment manager available:", unity.get_env_man() !== 0);
