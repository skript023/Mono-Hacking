# TypeScript examples

QuickJS in this project runs JavaScript. The `.ts` file is source for editing and type checking; transpile it to a plain `.js` file before loading it from the JavaScript console.

From the repository root, with TypeScript installed:

```powershell
npx tsc --target ES2020 --module none --lib ES2020,DOM --skipLibCheck --outDir .\scripts .\docs\typescript\valheim_mono.ts
```

The output is `scripts/valheim_mono.js`. The declarations at the top of the example exist only for TypeScript and are removed by the compiler. The runtime names (`unity`, `mono`, `Vector3`, and `console`) come from this project's C++ bindings.

The detour example is an ES module because it imports the built-in `detour` module:

```powershell
npx tsc --target ES2020 --module ES2020 --lib ES2020,DOM --skipLibCheck --outDir .\scripts .\docs\typescript\detour.d.ts .\docs\typescript\valheim_achievement_hook.ts
```

It demonstrates a pass-through hook: it calls the original method, logs the result, and returns that result unchanged. The native argument count is `1` for an instance method because the compiled call includes `this`; use `0` if the method is static. Verify the target method's actual class, overload, and static/instance status against the running game's assembly before enabling the hook.

The earlier QuickJSpp and detour changes update the native-to-JavaScript binding layer. They do not require ordinary scripts that use the same globals to be rewritten. Code using changed callback or hook signatures should still be checked against the current bindings.
