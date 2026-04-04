<p align="center">
  <img width="256" alt="C++ Logo" src="https://upload.wikimedia.org/wikipedia/commons/1/18/ISO_C%2B%2B_Logo.svg">
</p>

<h1 align="center">Valheim Hacks</h1>

<p align="center">
  <img src="https://img.shields.io/github/actions/workflow/status/skript023/Mono-Hacking/main.yml?branch=main&label=build&logo=github" />
  <img src="https://img.shields.io/github/actions/workflow/status/skript023/Mono-Hacking/nightly.yml?label=nightly&logo=github" />
  <img src="https://img.shields.io/github/v/release/skript023/Mono-Hacking?label=release&logo=github" />
</p>

<p align="center">
  A C++ framework for exploring and reverse engineering <b>Mono / Unity</b>-based games.<br>
  Built for direct interaction with the Mono runtime.<br><br>
  <b>⚠️ Strictly for educational and research purposes only.</b>
</p>

---

## 📌 Overview

**Valheim Hacks** is a lightweight internal framework designed to interface directly with the Mono runtime used in Unity games.

It provides a clean abstraction layer over Mono APIs, allowing native C++ code to interact with managed C# code seamlessly.

With this framework, you can:

- Access classes, methods, and fields from C#
- Invoke managed methods directly from C++
- Read and manipulate in-game objects (Player, Transform, etc.)
- Explore internal structures of Unity games
- Execute runtime scripts using embedded JavaScript (QuickJS)

---

## 🧩 Core Framework Features

These are the reusable building blocks of the framework, independent from any specific game logic.

- 🔍 Mono reflection helpers (class, method, field)
- 🧠 Clean wrappers for MonoObject, MonoClass, MonoMethod
- 🎯 Direct method invocation (`mono_runtime_invoke`)
- 📦 Utility conversions (e.g., `List<T>` → `std::vector`)
- 🧵 Worker loop system (tick/update handler)
- 🔌 Hook-ready architecture (easy to extend)
- 🖥️ Optional ImGui debug interface
- 📜 Embedded JavaScript engine (QuickJS)
- ⚡ Live scripting without recompilation
- 🧩 Modular and extensible design

---

## 🎮 Game Modules (Valheim)

These features are built on top of the framework and are specific to game behavior manipulation.

### 👤 Player (Self)

- Flying
- Infinite stamina
- No weight
- No item drop on death
- Always wind for sailing
- Teleport with any item
- Skill / stats modifiers (health, stamina, armor, etc.)

---

### 🎒 Inventory

- Override item properties:
  - Durability
  - Quality
  - Stack size
  - Variant
- Custom drop amount

---

### 🌍 World

- Tame all animals (deer, boar, wolf)
- World interaction utilities

---

### 👥 Online Players

- Player list retrieval via Mono
- Dynamic submenu generation per player
- Extendable for player-specific actions

---

### 👁️ ESP

- Player/entity ESP
- Name, health, box rendering
- Line & skeleton visualization

---

### 🎯 Aimbot

- Aimbot with configurable:
  - FOV
  - Smoothness
  - Trigger key
- Triggerbot
- FOV visualization
- Optional teleport targeting

---

### 📍 Teleport System

- Forward teleport
- Custom teleport system
- Save/load locations by category
- Persistent teleport storage
- Rotation-aware teleportation

---

## 🧠 Architecture Overview

The project is divided into two main layers:

### Core Layer
Handles:
- Mono interaction
- Memory / hooks
- Scripting engine
- UI system

### Feature Layer
Handles:
- Game-specific logic
- Player modifications
- ESP / Aimbot / Teleport
- High-level gameplay manipulation

---

## 🧵 Example: Dynamic UI System

```cpp
canvas::add_submenu<regular_submenu>("ESP", "SubmenuESP"_hash, [](regular_submenu* sub)
{
    sub->add_option<bool_option<bool>>("esp_activate"_hash);
    sub->add_option<bool_option<bool>>("draw_name"_hash);
    sub->add_option<bool_option<bool>>("draw_health"_hash);
});
```

## 📜 JavaScript Scripting (QuickJS)

This project integrates **QuickJS**, enabling runtime scripting using JavaScript.

This allows rapid prototyping and dynamic interaction with the game without rebuilding the project.

### Capabilities

- Call exposed native C++ bindings
- Interact with Mono classes and objects
- Execute scripts on-the-fly
- Build features faster before porting to C++

### Example

```js
// Hooking Example
detour.add(
  "Terminal::ConsoleCommand::IsValid", address, (ctx) => {
    console.log("ConsoleCommand", ctx.getArg(0).number().toString(16).toUpperCase());
    console.log("Terminal", ctx.getArg(1).number().toString(16).toUpperCase());
    console.log("Boolean", ctx.getArg(2).number().toString(16).toUpperCase());
    return true;
  }
);

detour.enable("Terminal::ConsoleCommand::IsValid");

// Access Max Health Example
const klass = mono.get_class("Player", "assembly_valheim");
console.log("class:", klass);
const field = mono.get_field(klass, "m_baseHP");
console.log("field:", field);
const player = unity.get_local_player();
console.log("player:", player);

mono.set_field_float(player, field, 800.0);

// Add tab menu example
import * as canvas from "canvas";

canvas.add_tab("Test2", 45, sub => {
  sub.add_bool("Flying");
  sub.add_slider("Max HP", 1, 500, 10);
  sub.add_choose("Mode", ["A", "B"], (i, v) => {
      console.log(i, v);
  });
});
```