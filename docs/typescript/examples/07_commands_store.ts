// API: commands.call, CommandFactory, BoolCommand, LoopedCommand, and store.
import { call } from "commands";
import { CommandFactory } from "command_types";

call("known_command_name"); // Unknown command names are ignored by the binding.

const toggle = CommandFactory.create_bool(
  "typescript_example_toggle",
  "TypeScript example toggle",
  "Example command",
  false,
);
toggle.on_enable(() => console.log("Toggle enabled"));
toggle.on_disable(() => console.log("Toggle disabled"));
console.log("Initial toggle state:", toggle.get_state());
// Set the state explicitly if you want this example command to change state:
// toggle.set_state(true);

const loop = CommandFactory.create_looped(
  "typescript_example_loop",
  "TypeScript example loop",
  "Runs its callback while enabled",
);
loop.on_tick(() => console.log("Loop tick"));
loop.on_disable(() => console.log("Loop disabled"));
console.log("Initial loop state:", loop.get_state());
// loop.set_state(true);

const key = "typescript_example_value";
store.set_bool(`${key}_bool`, true);
store.set_int(`${key}_int`, 7);
store.set_float(`${key}_float`, 2.5);
console.log(
  store.get_bool(`${key}_bool`),
  store.get_int(`${key}_int`),
  store.get_float(`${key}_float`),
);

