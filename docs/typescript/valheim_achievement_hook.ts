import { Hook } from "detour";

const hookName = "Achievement.CheckUnlocked";
const address = mono.get_compile_method("Achievement", "CheckUnlocked", 0, "assembly_valheim");

if (address === 0) {
	console.error("Achievement.CheckUnlocked was not found in assembly_valheim.");
} else {
	// For an instance method, include the implicit `this` pointer in the native argument count.
	const nativeArgumentCount = 1;
	const registered = Hook.add(hookName, address, nativeArgumentCount, (context) => {
		const result = context.callOriginal();
		console.log("Achievement.CheckUnlocked returned:", result !== 0);
		return result;
	});

	if (registered) {
		Hook.enable(hookName);
		console.log("Hook enabled:", hookName);
	} else {
		console.error("Could not register hook:", hookName);
	}
}
