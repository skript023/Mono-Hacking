import { Hook } from "detour";

const hookName = "Achievements.CanGetAchievements";
const address = mono.get_compile_method("Achievements", "CanGetAchievements", 1, "assembly_valheim");

if (address === 0) {
  console.error("Achievements.CanGetAchievements was not found in assembly_valheim.");
} else {
	// This static method has one bool parameter; it has no implicit `this` argument.
	const nativeArgumentCount = 1;
	const registered = Hook.add(hookName, address, nativeArgumentCount, (context) => {
		console.log("CanGetAchievements cheated argument:", context.getArg(0).number() !== 0);
		return context.callOriginal();
	});

	if (registered) {
		Hook.enable(hookName);
		console.log("Hook enabled:", hookName);
	} else {
		console.error("Could not register hook:", hookName);
	}
}
