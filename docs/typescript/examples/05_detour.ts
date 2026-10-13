// API: Hook.add/enable/disable and NativeContext / NativeArgument.
import { Hook, type NativeArgument, type NativeContext } from "detour";

const hookName = "example.Player.GetCurrentBiome";
const address = mono.get_compile_method("Player", "GetCurrentBiome", 0, "assembly_valheim");

function passThrough(context: NativeContext): number {
  // The implicit `this` pointer is native argument zero for an instance method.
  const self = context.getArg(0).number();
  const allRawArguments = context.args();
  console.log("Player pointer:", self);
  console.log("Raw arguments:", allRawArguments);
  const originalResult = context.callOriginal();
  return originalResult;
}

if (address !== 0) {
  // No explicit managed parameters, plus the implicit `this` pointer.
  const added = Hook.add(hookName, address, 1, passThrough);
  if (added) Hook.enable(hookName);
  else console.error("Could not add hook:", hookName);
}

// Call Hook.disable(hookName) when this hook should be removed.

function removeExampleHook(): void {
  Hook.disable(hookName);
}

// Use exactly one reader that matches the native parameter's real type.
function readIntegerOrPointer(argument: NativeArgument): number {
  return argument.number();
}
function readFloatArgument(argument: NativeArgument): number {
  return argument.float();
}
function readDoubleArgument(argument: NativeArgument): number {
  return argument.double();
}
function readCStringArgument(argument: NativeArgument): string {
  return argument.string();
}
function readMonoStringArgument(argument: NativeArgument): string | null {
  return argument.monoString();
}
function readValueOfArgument(argument: NativeArgument): number {
  return argument.valueOf();
}

// Call only for parameters whose position and type are verified in the target ABI.
function replaceExampleArguments(context: NativeContext): void {
  context.setArg(0, 0);
  context.setFloat(1, 1.5);
  context.setDouble(2, 2.5);
}
