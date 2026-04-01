<p align="center">
  <img width="256" alt="C++ Logo" src="https://upload.wikimedia.org/wikipedia/commons/1/18/ISO_C%2B%2B_Logo.svg">
</p>

<h1 align="center">Mono Game Hacking Base</h1>

<p align="center">
  Base framework untuk eksplorasi & reverse engineering game berbasis <b>Mono / Unity</b> menggunakan C++.<br>
  <b>Strictly for educational purposes only.</b>
</p>

---

## 📌 Overview

Project ini dibuat sebagai base internal untuk berinteraksi langsung dengan runtime Mono pada game berbasis Unity.

Dengan framework ini, kamu bisa:
- Akses class, method, dan field dari C#
- Invoke method langsung dari native C++
- Manipulasi object (Player, Transform, dll)
- Eksplorasi struktur internal game

Cocok untuk:
- Reverse engineering Unity games
- Research Mono runtime
- Internal debugging & experimentation

---

## ⚙️ Features

- 🔍 Mono reflection helper (class, method, field)
- 🧠 Wrapper untuk MonoObject, MonoClass, dll
- 📦 Utility conversion (List → std::vector)
- 🎯 Direct method invoke (mono_runtime_invoke)
- 🧵 Worker loop (update/tick system)
- 🖥️ ImGui debug menu (optional)
- 🔌 Hook-ready structure

---

## 🧠 Basic Concept (Mono)

Game Unity berbasis Mono memiliki 2 layer utama:

- Managed Layer → C# (Assembly-CSharp.dll)
- Native Layer → Engine (C++)

Framework ini bekerja dengan bridge ke Mono runtime:

- mono_get_root_domain
- mono_thread_attach
- mono_class_from_name
- mono_class_get_method_from_name
- mono_runtime_invoke

### Example

```cpp
auto klass = mono::get_class("Player", "Assembly-CSharp");
auto method = mono::get_method(klass, "GetPosition", 0);
auto result = mono::invoke(method, instance);
```