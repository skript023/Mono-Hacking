# Audit Bug Valheim (Mono-Hacking)

Tanggal: 2026-10-21, diperbarui 2026-10-23. Dicek pada branch `shared`, commit `6651dda`.

Cara cek: kode mod dibaca lalu dibandingkan dengan IL `assembly_valheim.dll` dari game yang terpasang
(`E:\SteamLibrary\steamapps\common\Valheim`), lewat runtime Mono milik game. Nilai default game diambil
dari konstruktor field di build sekarang. Nilai yang disimpan di prefab Unity tidak bisa dibaca dengan cara
ini; temuan yang bergantung pada nilai prefab ditandai **belum dipastikan**. Belum ada yang dites di dalam game.

## Ringkasan

| # | Masalah | Dampak | Lokasi |
| --- | --- | --- | --- |
| 1 | Max Eitr ditimpa game setiap detik, eitr ikut terpotong | Tinggi, diperbaiki | `features/max_mana.cpp` |
| 2 | Mematikan No Wave & Collision Damage mengisi nilai yang salah | Sedang, diperbaiki | `features/ship.cpp` |
| 3 | Ashlands Ocean Boiling Immunity tidak bisa dimatikan | Sedang, diperbaiki | `features/ship.cpp` |
| 4 | Ship Speed Boost memakai angka tetap untuk semua kapal | Sedang, belum dipastikan, diperbaiki | `features/ship.cpp` |
| 5 | Mematikan Disable Base Raids mengisi peluang raid yang salah | Rendah, diperbaiki | `features/world.cpp` |
| 6 | Mematikan fitur fishing mengisi nilai yang salah | Rendah, diperbaiki | `features/fishing.cpp` |
| 7 | Never Wet berlaku untuk semua karakter dan juga menghapus efek Tar | Sedang, diperbaiki | `hooks/update_water.cpp` |
| 8 | Enam opsi menu tidak dipakai kode mana pun | Sedang, diperbaiki | beberapa file |
| 9 | Label Food Duration Multiplier terbalik dengan perilakunya | Rendah, diperbaiki | `menu/submenu/player_menu.cpp` |
| 10 | Max Body Armor menyala secara default dengan nilai 999 | Info, diperbaiki | `hooks/get_body_armor.cpp` |
| 11 | Enam hook berisiko tidak terpanggil karena inline JIT | Dicek: tidak terjadi | `hooking.cpp` |
| 12 | Speed fermenter dan tanaman bergantung pada jarak pemain | Rendah, fermenter diperbaiki | `hooks/base_tools.cpp` |
| 13 | Noclip macet setelah mati atau ganti world, dan melempar exception setiap frame | Sedang, diperbaiki | `hooks/player_update.cpp` |
| 14 | Infinite Durability mengisi durability 1000, bukan nilai maksimal item | Sedang, diperbaiki | `hooks/durability.cpp` |
| 15 | Makanan dan mead dari slot food tidak pernah berkurang | Tinggi, diperbaiki | `unity/equipment_slots.cpp` |
| 16 | `HaveEmptySlot` selalu true untuk semua inventory, loot peti kapal bisa hilang | Sedang, diperbaiki | `hooks/have_empty_slot.cpp` |
| 17 | Inventory Size menyembunyikan item saat dimatikan dan membuat item jatuh saat spawn tanpa mod | Sedang, tidak dikerjakan | `features/inventory.cpp` |
| 18 | Hook kosong dan log yang tidak perlu | Rendah, diperbaiki | beberapa file |
| 19 | Unload tidak mematikan fitur looped, jadi nilai yang diubah tetap tertinggal di game | Tinggi, diperbaiki | `main.cpp`, `commands/commands.cpp` |
| 20 | Toggle fitur dan beberapa tombol menu memanggil API Unity dari thread render | Sedang, sebagian belum dipastikan, diperbaiki | `gui.cpp`, beberapa menu |
| 21 | Prevent Freezing/Cold/Wet dan Auto-Purge bertarung dengan game setiap frame | Sedang, diperbaiki | `hooks/player_update.cpp`, `features/buffs.cpp` |
| 22 | Exception Mono ditelan tanpa log | Rendah, diperbaiki | `services/mono/mono.cpp` |
| 23 | Jendela Equipment, Bookmark, dan menu skrip JS menjalankan aksi game dari thread render | Sedang, belum dipastikan, diperbaiki kecuali JS | `unity/equipment_slots.cpp`, `unity/bookmark_manager.cpp`, `services/javascript` |
| 24 | Hook aimbot tidak mengecek penembak, jadi serangan creature dan semua proyektil ikut diarahkan | Sedang, diperbaiki | `hooks/start_attack.cpp`, `hooks/on_hit.cpp` |
| 25 | Item di slot equipment tidak pernah masuk tombstone, dan tidak terlihat tanpa mod | Sedang, disengaja (deskripsi ditambah) | `unity/equipment_slots.cpp` |
| 26 | Return All melaporkan berhasil walaupun inventory penuh, dan armor sudah terlepas | Rendah, diperbaiki | `unity/equipment_slots.cpp` |
| 27 | Jalur exception SEH di `drop_item` dan `parry_hooks` | Rendah, parry diperbaiki | `hooks/drop_item.cpp`, `hooks/parry_hooks.cpp` |
| 28 | Makanan di slot tambahan (Max Food Slots) tidak dihitung ke stat makanan | Rendah, diperbaiki | `hooks/eat_food.cpp` |
| 29 | Remote Trader memasang animator karakter ke prefab trader, lalu beli dan jual melempar exception setelah mati | Sedang, sebagian belum dipastikan, diperbaiki | `unity/world_tools.cpp` |
| 30 | Allow Picking Up Fish mengubah fisika dan gerak semua ikan | Rendah, diperbaiki | `hooks/is_out_of_water.cpp` |
| 31 | No Weight baru berlaku setelah isi inventory berubah, dan membuat gerobak tanpa bobot | Rendah | `hooks/get_weight.cpp` |
| 32 | Builder submenu berjalan setiap frame: Custom Teleport membocorkan memori, Guardian Powers punya race dengan thread game | Sedang | `menu/submenu/teleport_submenu.cpp`, `menu/submenu/player_menu.cpp`, Astra |
| 33 | `ItemData.m_worldLevel` dibaca dan ditulis sebagai 1 byte, padahal `int` | Rendah | `unity/item_spawner.cpp`, `unity/inventory_editor.cpp` |
| 34 | Tiga lookup memakai nama field yang sudah tidak ada di game | Rendah | `unity/procreation.cpp`, `unity/tameable.cpp`, `utility/unity.hpp` |
| 35 | Masalah kecil di Prefab Spawner, Creature Spawner, dan custom teleport | Rendah | `unity/prefab_spawner.cpp`, `unity/creature_spawner.cpp`, `services/custom_teleport` |
| 36 | Grid equipment native kemungkinan hanya menampilkan item pertama di build sekarang | Sedang, belum dites di game | `unity/equipment_slots.cpp` |
| 37 | Unload tidak menghapus grid equipment, dan item yang dipindah lewat grid itu bisa hilang atau terduplikasi | Sedang | `unity/equipment_slots.cpp`, `main.cpp` |
| 38 | Tame All in Radius dan Tame Aimed Creature ikut mengambil alih karakter pemain lain | Tinggi di multiplayer | `unity/character.cpp`, `unity/animal_tools.cpp` |
| 39 | Teleport XYZ, crosshair, dan pin mengabaikan nilai Y | Rendah | `utility/unity.hpp` |
| 40 | Kesalahan kecil di helper: `can_pickup`/`can_eat` tidak di-unbox, dan pin type salah di fungsi yang tidak dipakai | Rendah | `unity/item_drop.cpp`, `utility/unity.hpp` |
| 41 | Consolidate Stacks di Inventory Editor tidak pernah jalan | Rendah | `unity/inventory_editor.cpp` |
| 42 | Masalah kecil di exploration, breeding, dan fishing: pin The Queen ikut di Infested Mine, Breeding Speed hanya separuh berlaku | Rendah | `unity/exploration.cpp`, `unity/procreation.cpp`, `unity/fishing_tools.cpp` |
| 43 | World Time Scale dan Pause Time hilang setiap kali menu ESC ditutup, dan label menu tidak ikut berubah | Rendah | `unity/world_time.cpp` |
| 44 | Sepuluh nama prefab tidak ada di game: sebagian Quick Spawn dan pin Sealed Tower tidak pernah berhasil | Rendah | `menu/submenu/player_menu.cpp`, `unity/item_spawner.cpp`, `unity/exploration.cpp` |
| 45 | Task async yang kena exception SEH dijalankan ulang setiap frame | Rendah, belum dipastikan | `task_mgr.cpp`, `logger/exception_handler.cpp`, `logger/stack_trace.cpp` |
| 46 | Masalah kecil di World Modifiers, Tame All, scanner pattern JS, settings, dan kode mati | Rendah | beberapa file |
| 47 | Login dan WebSocket server mematikan verifikasi sertifikat TLS, jadi token dan data hardware bisa disadap | Sedang (keamanan), hanya build `PRODUCTION` | `server/auth_client.hpp`, `server/socket_client.hpp` |
| 48 | Hook dari skrip JS dihapus dua kali saat unload, dan callback-nya memakai QuickJS dari thread mana saja | Sedang, hanya kalau skrip memakai `detour.add` | `services/javascript/bindings/trampoline_binding.cpp`, `hooking.cpp` |
| 49 | Bug di binding JS: `set_field_string`, `get_field_value`, argumen float, slider integer, dan `canvas.add_submenu` | Rendah | `services/javascript/bindings/` |
| 50 | Tombol Exit di Settings menutup game tanpa menyimpan world dan karakter | Sedang | `menu/submenu/setting_menu.cpp` |
| 51 | Masalah kecil di server, input, teleport, dan renderer Vulkan | Rendah, sebagian belum dipastikan | beberapa file |

Temuan performa ada di bagian [Performa](#performa).

Sudah diperbaiki sebelumnya: Attack Animation Speed menimpa `Animator.speed` setiap frame walaupun
pengalinya 1.0, sehingga tempo serangan dan hit-stop vanilla hilang. Diperbaiki di `557a25e`.

Diperbaiki pada 2026-10-23: #1 sampai #10, #12 sampai #16, #18 sampai #30, dan P1 sampai P15. #11 sudah dicek
dan ternyata tidak terjadi. #17 tidak dikerjakan atas keputusan pemilik. Sebagian #23 dan #27 sengaja dilewati;
alasannya ada di bagian masing-masing. Build `x64-Clang-Release` berhasil, tetapi perbaikannya belum dites di
dalam game. Cara perbaikannya ada di baris **Status** pada masing-masing bagian.

## Detail

### 1. Max Eitr ditimpa game setiap detik

- Lokasi: [max_mana.cpp:36](../src/features/max_mana.cpp#L36), yang memanggil
  [player.cpp:28](../src/unity/player.cpp#L28) untuk menulis `Player.m_maxEitr`.
- Penyebab: `m_maxEitr` adalah nilai hasil hitungan. `Player.UpdateFood` memanggil `SetMaxEitr` setiap
  1 detik dengan total eitr dari makanan, dan `SetMaxEitr` langsung menjepit `m_eitr` ke batas baru itu.
  `GetTotalFoodValue` memakai `m_baseHP` dan `m_baseStamina`, tetapi tidak punya nilai base untuk eitr.
- Akibat: bar eitr terlihat penuh sesuai setting, tetapi isinya dipotong kembali ke batas makanan setiap
  detik (0 kalau tidak ada makanan eitr). Max Health dan Max Stamina tidak kena, karena keduanya menulis
  field base.
- Saran: jangan menulis `m_maxEitr`. Hook `Player.SetMaxEitr(float, bool)` dan ganti argumennya selama fitur
  menyala, supaya game sendiri yang menyimpan nilai dan menjepit `m_eitr` dengan batas yang benar.
- **Status**: diperbaiki. Hook baru [set_max_eitr.cpp](../src/hooks/set_max_eitr.cpp) mengganti argumen
  `SetMaxEitr` dengan nilai setting selama fitur menyala dan mencatat nilai dari makanan. `SetMaxEitr` hanya
  dipanggil `UpdateFood` dan `Player.Load`, dan hanya method itu yang menulis `m_maxEitr`, jadi hook melihat
  setiap nilai yang dihitung game. Tulisan langsung di `on_tick` tetap ada supaya nilai langsung berlaku saat
  fitur dinyalakan, termasuk saat Food Duration Multiplier 0 (`UpdateFood` tidak memanggil `SetMaxEitr`).
  Saat fitur dimatikan, nilai makanan terakhir dikembalikan dan `m_eitr` dijepit ke batas itu, di thread game
  lewat `async_run`.

### 2. Mematikan No Wave & Collision Damage mengisi nilai yang salah

- Lokasi: [ship.cpp:68-97](../src/features/ship.cpp#L68-L97).
- Penyebab: `on_disable` mengisi angka tetap: `m_waterImpactDamage = 1` dan `m_upsideDownDmg = 10`.
  Default game adalah `10` dan `20`.
- Akibat: setelah fitur dimatikan, kapal menerima damage 10x lebih kecil dari ombak dan benturan, dan 2x
  lebih kecil saat terbalik, sampai objek kapal dimuat ulang.
- Saran: rekam nilai asli per kapal saat pertama kali ditimpa, lalu kembalikan nilai itu.
- **Status**: diperbaiki bersama #3 dan #4. `ship_override` di [ship.cpp](../src/features/ship.cpp) mencatat
  nilai asli setiap kapal saat pertama kali ditimpa (kapal ditahan lewat GC handle), lalu memulihkan semua kapal
  itu saat fitur dimatikan, termasuk kapal yang sudah ditinggal. Pemulihan berjalan di thread game lewat
  `async_run`, jadi tidak berebut data dengan `on_tick`.

### 3. Ashlands Ocean Boiling Immunity tidak bisa dimatikan

- Lokasi: [ship.cpp:10-31](../src/features/ship.cpp#L10-L31).
- Penyebab: `m_ashlandsReady` diset `true` setiap tick, dan class ini tidak punya `on_disable`.
- Akibat: kapal tetap kebal terhadap air Ashlands setelah fitur dimatikan, sampai objek kapal dimuat ulang.
- Saran: rekam nilai asli per kapal dan kembalikan di `on_disable`.
- **Status**: diperbaiki dengan cara yang sama seperti #2. `m_ashlandsReady` hanya dibaca
  `TakeAshlandsDamage` dan tidak pernah ditulis game, jadi nilai aslinya berasal dari prefab dan sekarang
  dicatat sebelum ditimpa.

### 4. Ship Speed Boost memakai angka tetap untuk semua kapal (belum dipastikan)

- Lokasi: [ship.cpp:102-166](../src/features/ship.cpp#L102-L166).
- Penyebab: `m_sailForceFactor`, `m_stearForce`, `m_backwardForce`, dan `m_rudderSpeed` ditulis sebagai
  `0.1 / 0.5 / 50 / 0.5` dikali pengali, dan angka yang sama dipakai di `on_disable`. Angka ini sama dengan
  default di konstruktor, tetapi rakit, karve, longship, dan drakkar bisa punya nilai sendiri di prefab.
- Akibat: kalau nilai prefab berbeda, handling kapal berubah walaupun pengali 1x, dan tetap berubah setelah
  fitur dimatikan. Hanya kapal yang sedang dikemudikan yang dipulihkan; kapal yang ditinggal saat fitur
  menyala tetap memakai nilai mod.
- Saran: rekam nilai asli per kapal, kalikan nilai itu, dan kembalikan saat dimatikan.
- **Status**: diperbaiki dengan cara yang sama seperti #2. Pengali sekarang dikalikan ke nilai asli kapal itu
  (batas 3x untuk stear dan 2,5x untuk rudder tetap), jadi pengali 1x tidak mengubah handling.

### 5. Mematikan Disable Base Raids mengisi peluang raid yang salah

- Lokasi: [world.cpp:43](../src/features/world.cpp#L43).
- Penyebab: `on_disable` mengisi `m_eventChance = 20`. Default game adalah `25`.
- Akibat: peluang raid tetap lebih rendah dari vanilla sampai world dimuat ulang. Hanya berpengaruh di host,
  karena `RandEventSystem` memutuskan raid di sana.
- Saran: rekam nilai asli sebelum diisi 0, lalu kembalikan nilai itu.
- **Status**: diperbaiki. Nilai asli dicatat per instance `RandEventSystem` dan dikembalikan saat fitur
  dimatikan, atau saat instance berganti karena world dimuat ulang.

### 6. Mematikan fitur fishing mengisi nilai yang salah

- Lokasi: [fishing.cpp:50](../src/features/fishing.cpp#L50) dan
  [fishing.cpp:107](../src/features/fishing.cpp#L107).
- Penyebab: Unbreakable Fishing Line mengisi `m_breakDistance = 30` (default `4`). Zero Stamina Fishing
  mengisi `m_pullStaminaUse = 15` (default `10`).
- Akibat: senar hampir tidak bisa putus dan tarikan memakan stamina 50% lebih banyak, tetapi hanya pada
  pelampung yang sedang dilempar saat fitur dimatikan. Lemparan berikutnya membuat pelampung baru.
- Saran: rekam nilai asli per pelampung, atau biarkan pelampung yang sedang aktif tanpa diubah.
- **Status**: diperbaiki. `ship_override` dari #2 dijadikan helper umum
  [field_override.hpp](../src/features/field_override.hpp) dan dipakai juga oleh kedua fitur fishing: nilai
  asli dicatat per pelampung dan dipulihkan saat fitur dimatikan. Pelampung yang sudah hilang dari
  `FishingFloat.m_allInstances` dilepas setiap tick, jadi GC handle tidak menumpuk selama sesi memancing.

### 7. Never Wet berlaku untuk semua karakter dan juga menghapus efek Tar

- Lokasi: [update_water.cpp:6-33](../src/hooks/update_water.cpp#L6-L33).
- Penyebab: hook `Character.UpdateWater` tidak mengecek pemain lokal, dan mengenolkan `m_waterLevel` serta
  `m_tarLevel` sebelum fungsi asli berjalan.
- Akibat: semua karakter yang dikontrol client Anda (musuh, hewan jinak) ikut kebal efek Wet dan Tarred.
  Pemain juga kebal Tarred, padahal nama fiturnya hanya menyebut basah. Berenang tidak terpengaruh, karena
  status renang memakai `m_cashedInLiquidDepth` dan `m_swimTimer`.
- Saran: tambahkan cek `_this == unity::get_local_player()`, lalu putuskan apakah Tar memang ikut dicegah
  (dan sesuaikan nama atau deskripsinya).
- **Status**: diperbaiki. Hook hanya berlaku untuk pemain lokal. Atas keputusan pemilik, Tar tetap dicegah,
  dan opsinya diganti menjadi "Never Wet or Tarred" dengan deskripsi yang menyebut Tar. Yang dicegah hanya Tar
  dari genangan; efek Tarred dari serangan musuh tidak terpengaruh.

### 8. Enam opsi menu tidak dipakai kode mana pun

| Opsi | Definisi | Menu | Catatan |
| --- | --- | --- | --- |
| Retain Skills on Death | [world.cpp:50](../src/features/world.cpp#L50) | [player_menu.cpp:1909](../src/menu/submenu/player_menu.cpp#L1909) | Default menyala. [raise_skill.cpp:32-50](../src/hooks/raise_skill.cpp#L32-L50) hanya mengecek `freeze_skills` dan `prevent_skill_loss`, jadi skill tetap turun saat mati. |
| Draw Animation | [features.hpp:19](../src/features/features.hpp#L19) | [player_menu.cpp:1568](../src/menu/submenu/player_menu.cpp#L1568) | Tidak dibaca. |
| Aimbot Trigger Type | [features.hpp:31](../src/features/features.hpp#L31) | [player_menu.cpp:1588](../src/menu/submenu/player_menu.cpp#L1588) | Tidak dibaca. |
| Aimbot Smooth | [features.hpp:33](../src/features/features.hpp#L33) | [player_menu.cpp:1587](../src/menu/submenu/player_menu.cpp#L1587) | Tidak dibaca. |
| Triggerbot | [features.hpp:34](../src/features/features.hpp#L34) | [player_menu.cpp:1589](../src/menu/submenu/player_menu.cpp#L1589) | Tidak dibaca. |
| Trigger FOV | [features.hpp:35](../src/features/features.hpp#L35) | [player_menu.cpp:1590](../src/menu/submenu/player_menu.cpp#L1590) | Tidak dibaca. |

- Saran: sambungkan `keep_skills_on_death` ke `hooks::lower_all_skills`. Opsi lain disembunyikan dari menu
  sampai ada kode yang memakainya.
- Command tanpa pemakai yang tidak muncul di menu (tidak mengganggu pemain): `disable_input`,
  `draw_box_3d`, `draw_corner_box`, `draw_skeleton`, `draw_team`.
- **Status**: diperbaiki. `keep_skills_on_death` dipindah ke `features.hpp` dan sekarang dicek di
  `hooks::lower_all_skills`. `LowerAllSkills` hanya dipanggil `Skills.OnDeath` dari `Player.OnDeath`, jadi opsi
  ini tidak memengaruhi penurunan skill lain. Default-nya tetap menyala, sehingga skill tidak lagi turun saat
  mati. Lima opsi lain dihapus dari menu, tetapi command-nya tetap ada, supaya nilai di `settings.json` tidak
  hilang.

### 9. Label Food Duration Multiplier terbalik dengan perilakunya

- Lokasi: [player_menu.cpp:526](../src/menu/submenu/player_menu.cpp#L526) dan
  [update_food.cpp:32](../src/hooks/update_food.cpp#L32).
- Penyebab: kode mengalikan kecepatan berkurangnya timer makanan. Deskripsinya sudah benar ("Scale how
  quickly food timers decay"), tetapi labelnya berbunyi "Duration".
- Akibat: nilai 2.0 membuat makanan habis dua kali lebih cepat, kebalikan dari yang diharapkan dari label.
- Saran: ganti label menjadi "Food Decay Speed", atau ubah rumusnya supaya 2.0 berarti dua kali lebih awet.
- **Status**: diperbaiki dengan mengubah rumusnya, sesuai keputusan pemilik. Timer makanan sekarang maju
  `1 / pengali` per detik, jadi 2.0 berarti makanan dua kali lebih awet, 0.5 dua kali lebih cepat habis, dan 0
  tetap berarti tidak berkurang. Deskripsi di menu ikut diganti. Nilai yang sudah tersimpan di `settings.json`
  berbalik artinya.

### 10. Max Body Armor menyala secara default dengan nilai 999

- Lokasi: [get_body_armor.cpp:7](../src/hooks/get_body_armor.cpp#L7), menu
  [player_menu.cpp:493](../src/menu/submenu/player_menu.cpp#L493).
- Keterangan: ini pilihan default, bukan kesalahan kode. Di `settings.json` saat audit, opsi ini juga masih
  menyala, jadi armor pemain selalu 999 selama mod aktif.
- Saran: pertimbangkan default mati, supaya perilaku game hanya berubah kalau pemain menyalakannya.
- **Status**: diperbaiki. Default `max_body_armor` sekarang mati. Perubahan ini hanya berlaku kalau
  `settings.json` belum menyimpan nilainya. Kalau sudah tersimpan menyala, opsinya tetap menyala sampai
  dimatikan lewat menu.
- Sudah dicek dan aman: `Player.TryGetArmorDifference` memanggil `GetBodyArmor()` dua kali lalu
  mengurangkannya, jadi perbandingan armor di tooltip tetap benar walaupun override menyala. Damage ke pemain
  lain dihitung di client mereka sendiri, jadi override ini hanya berlaku untuk karakter sendiri.

### 11. Enam hook berisiko tidak terpanggil karena inline JIT (belum dipastikan)

JIT Mono meng-inline method non-virtual yang IL-nya di bawah 20 byte. Di pemanggil yang di-inline, detour
tidak dipanggil.

| Hook | Ukuran IL | Call site di game | Registrasi |
| --- | --- | --- | --- |
| `CraftingStation.GetLevel` | 10 B | 5 | [hooking.cpp:120](../src/hooking.cpp#L120) |
| `Inventory.ContainsItem` | 13 B | 21 | [hooking.cpp:224](../src/hooking.cpp#L224) |
| `Player.IsKnownMaterial` | 13 B | 1 | [hooking.cpp:118](../src/hooking.cpp#L118) |
| `ZoneSystem.GetGlobalKey(GlobalKeys)` | 13 B | 53 | [hooking.cpp:119](../src/hooking.cpp#L119) |
| `WearNTear.HaveSupport` | 18 B | 2 | [hooking.cpp:126](../src/hooking.cpp#L126) |
| `Attack.GetAttackEitr()` | 19 B | 3 | [hooking.cpp:175](../src/hooking.cpp#L175) |

- Akibat yang mungkin: fitur terkait (level crafting station, recipe material, override global key,
  stabilitas bangunan, biaya eitr) kadang bekerja dan kadang tidak, tergantung jalur pemanggilnya.
- Saran: tes fitur-fitur ini di game. Kalau ada yang tidak konsisten, pindahkan hook ke method pemanggil
  yang lebih besar.
- **Status**: dicek, dan inline itu tidak terjadi. Lewat runtime Mono milik game, setiap method target
  di-compile JIT, lalu setiap pemanggilnya di `assembly_valheim` juga di-compile. Kode mesin pemanggil dicek
  apakah masih memanggil target, langsung atau lewat trampoline JIT yang menyimpan pointer `MonoMethod`.
  - Hasil: semua call site yang bisa di-compile di luar game masih berupa panggilan, tidak di-inline. Itu
    mencakup 1 untuk `IsKnownMaterial`, 2 untuk `HaveSupport`, 3 untuk `GetAttackEitr`, 5 untuk `GetLevel`,
    15 untuk `ContainsItem`, dan 17 untuk `GetGlobalKey(GlobalKeys)`.
  - Pembanding: `Player.SetMaxEitr` (71 byte) tetap berupa panggilan, sedangkan `ZNet.get_instance` (6 byte)
    di-inline di 168 call site. Jadi metode ini bisa membedakan keduanya.
  - Keenam target sama-sama memanggil method lain di dalamnya, dan JIT Mono di build ini tidak meng-inline method
    seperti itu. Aturan "di bawah 20 byte pasti di-inline" ternyata terlalu luas.
  - Sisa yang belum terlihat: 3 pemanggil `ContainsItem` dan 22 pemanggil `GetGlobalKey` tidak bisa di-compile
    di luar Unity karena membutuhkan internal call engine. Keputusan inline ditentukan oleh method target, jadi
    hasilnya hampir pasti sama, tetapi belum terbukti langsung.
  - Tidak ada perubahan kode.

### 12. Speed fermenter dan tanaman bergantung pada jarak pemain

- Lokasi: [base_tools.cpp:11-25](../src/hooks/base_tools.cpp#L11-L25) dan
  [base_tools.cpp:190-203](../src/unity/base_tools.cpp#L190-L203).
- Penyebab: `Fermenter.GetFermentationTime` dan `Plant.GetGrowTime` adalah waktu total, bukan delta.
  Pengali hanya dipakai saat pemain berada dalam radius dan client memegang objeknya.
- Akibat: status "siap" bisa kembali menjadi "belum siap" saat pemain keluar dari radius. Smelter, cooking
  station, beehive, dan sap collector tidak kena, karena hook-nya mengalikan delta.
- Saran: terima perilaku ini dan tulis di dokumentasi, atau simpan progres yang sudah dipercepat.
- **Status**: fermenter diperbaiki dengan menyimpan progresnya.
  - Hook `Fermenter.GetFermentationTime` dihapus. Sebagai gantinya, sekali per detik `base_tools` memundurkan
    `ZDOVars.s_startTime` setiap fermenter dalam radius yang dipegang client ini, sebesar
    `(pengali - 1) x waktu berlalu` dalam tick (`ZNet.m_netTime x 10^7`, sama dengan yang dipakai game).
  - Progres tetap ada saat pemain pergi, dan ikut tersinkron ke pemain lain karena tersimpan di ZDO.
  - Bedanya dengan perilaku lama: pengali sekarang hanya mempercepat waktu berikutnya. Waktu yang sudah lewat
    sebelum pemain datang tidak ikut dikalikan.
  - Tanaman tidak diubah. Begitu `Plant.Grow` berjalan, tanaman tidak bisa mundur lagi. Namun percepatannya
    tetap hanya berlaku selama pemain berada dalam radius.

### 13. Noclip macet setelah mati atau ganti world

- Lokasi: [player_update.cpp:96-107](../src/hooks/player_update.cpp#L96-L107).
- Penyebab: selama noclip menyala, collider lama disimpan sebagai pointer statis. Saat pemain mati atau
  pindah world, collider berganti. Mod mencoba mengembalikan `enabled` pada collider lama yang objek Unity-nya
  sudah dihancurkan, mendapat exception, lalu `return` sebelum `active_collider` diganti ke collider baru.
- Akibat: noclip tidak bekerja lagi pada karakter baru sampai toggle dimatikan dan dinyalakan ulang, dan
  setiap frame game melempar exception managed baru (mahal). Pointer statis itu juga tidak dijaga GC handle,
  jadi kalau objeknya sudah dikoleksi GC, memanggil method padanya berisiko crash.
- Saran: kalau restore gagal, tetap lepaskan pointer lama dan lanjut ke collider baru. Simpan collider
  dengan GC handle, seperti `retain_impl` di `hud.cpp`.
- **Status**: diperbaiki bersama P6. Collider disimpan lewat GC handle. Saat collider berganti, mod mencoba
  memulihkan yang lama, lalu melepasnya walaupun pemulihan gagal, dan langsung lanjut ke collider baru.

### 14. Infinite Durability mengisi durability 1000, bukan nilai maksimal item

- Lokasi: [durability.cpp:8-13](../src/hooks/durability.cpp#L8-L13) dan
  [attack_damage.cpp:128-133](../src/hooks/attack_damage.cpp#L128-L133).
- Penyebab: setiap kali game menguras durability item yang dipakai, dan setiap kali senjata mengenai sesuatu,
  mod menulis `m_durability = 1000`.
- Akibat: item yang maksimalnya di bawah 1000 (kebanyakan senjata, alat, dan obor) mendapat durability di
  atas batas. Nilai ini ikut tersimpan di data item, jadi tetap awet setelah fitur dimatikan. Item yang
  maksimalnya di atas 1000 (misalnya lewat Item Overrides) justru turun ke 1000.
- Saran: jangan menulis apa pun dan cukup lewati pengurasan, atau isi dengan `GetMaxDurability` item itu.
- **Status**: diperbaiki.
  - Hook `DrainEquipedItemDurability` sekarang hanya melewati pengurasan.
  - Setelah serangan mengenai sesuatu, senjata diisi ke durability maksimalnya sendiri (`item_data::get_max_durability`,
    termasuk override `ew_max_durability`). Pengurasan senjata ada di dalam `Attack.DoMeleeAttack`,
    `DoAreaAttack`, dan `ProjectileAttackTriggered`, jadi tidak bisa dilewati dari hook yang ada.
  - Item yang sudah terlanjur bernilai 1000 tidak dikoreksi otomatis. Memperbaikinya di workbench
    mengembalikannya ke nilai maksimal.

### 15. Makanan dan mead dari slot food tidak pernah berkurang

- Lokasi: [equipment_slots.cpp:814](../src/unity/equipment_slots.cpp#L814).
- Penyebab: hotkey slot food (Shift+1..3) memanggil `Humanoid.UseItem(player_inv, item, true)` dengan
  inventory utama, padahal item ada di inventory equipment. `UseItem` lolos karena hook `ContainsItem` juga
  mencari di inventory equipment. Lalu `Player.ConsumeItem` memanggil `EatFood` dan
  `inventory.RemoveOneItem(item)` pada inventory utama, dan hasilnya diabaikan. `RemoveOneItem` gagal karena
  item tidak ada di sana, dan method ini tidak di-hook.
- Akibat: makanan dimakan dan mead diminum, tetapi stack di slot tidak berkurang. Ini duplikasi item.
- Saran: kirim inventory equipment sebagai argumen pertama `UseItem`, atau hapus satu item dari inventory
  equipment setelah `UseItem` berhasil.
- **Status**: diperbaiki. Hotkey slot food sekarang mengirim inventory equipment ke `UseItem`. `UseItem` hanya
  memakai `m_inventory` kalau argumennya null, dan `Player.ConsumeItem` memanggil `RemoveOneItem` pada inventory
  yang diterimanya, jadi stack di slot berkurang. `use_slot` lalu menyimpan halaman equipment seperti biasa.

### 16. `HaveEmptySlot` selalu true untuk semua inventory

- Lokasi: `hooks/have_empty_slot.cpp:6-14` (file sudah dihapus).
- Penyebab: hook berisi `if (true) return true;`, tanpa mengecek setting, `g_running`, atau inventory mana.
- Akibat, berdasarkan pemanggil di game:
  - `Piece.DropResources`: untuk piece yang loot-nya dimasukkan ke peti (`m_destroyedLootPrefab`, misalnya
    kapal yang hancur), game membuat peti baru saat peti lama penuh. Dengan hook ini game terus mengisi peti
    yang sama, dan item yang tidak muat hilang karena hasil `AddItem` diabaikan.
  - `StoreGui.UpdateBuyButton`: tombol Buy tetap aktif walaupun inventory penuh. Pembelian tetap aman karena
    koin baru dipotong kalau `AddItem` berhasil.
- Saran: hapus hook ini, atau kaitkan dengan setting yang jelas dan batasi ke inventory pemain lokal.
- **Status**: diperbaiki. Hook dihapus. Di build game sekarang, `HaveEmptySlot` hanya dipanggil
  `StoreGui.UpdateBuyButton`, jadi tidak ada fitur mod yang bergantung padanya.

### 17. Inventory Size menyembunyikan item dan membuat item jatuh saat spawn tanpa mod

- Lokasi: [inventory.cpp:78](../src/features/inventory.cpp#L78),
  [inventory.cpp:111-128](../src/features/inventory.cpp#L111-L128), dan
  [drop_item.cpp:43-46](../src/hooks/drop_item.cpp#L43-L46).
- Penyebab:
  - Saat dimatikan, lebar inventory dikembalikan ke 8 tanpa mengecek kolom yang masih berisi item. Tinggi
    sudah dicek, lebar belum.
  - `invrows` disimpan sebagai `clamp(rows, 4, 9)`, padahal tinggi sebenarnya bisa sampai 1000.
  - Hook `drop_invalid_items` selalu memblokir `Humanoid.DropInvalidItems`.
- Akibat:
  - Item di kolom 9 ke atas jadi tidak terlihat dan tidak bisa dipakai setelah fitur dimatikan, tetapi tetap
    ada dan tetap dihitung beratnya.
  - `Player.OnSpawned` membaca `invrows`, memanggil `SetInventorySize`, lalu `DropInvalidItems`. Kalau karakter
    di-spawn saat mod tidak aktif (main tanpa mod, atau inject setelah spawn), semua item di luar 8 kolom dan
    `invrows` baris dijatuhkan ke tanah di lokasi spawn.
- Saran: saat dimatikan, pindahkan item di luar grid vanilla ke slot kosong (atau tolak mematikan kalau tidak
  muat), dan beri tahu pemain kalau inventory lebih besar dari 8x9 berisiko saat bermain tanpa mod.
- **Status**: tidak dikerjakan, atas keputusan pemilik. Catatan tambahan dari pengecekan: `Player.OnSpawned`
  vanilla tidak membatasi nilai `invrows`, jadi baris tambahan sebenarnya bisa dipertahankan tanpa mod kalau
  nilai aslinya disimpan. Lebar selalu 8 di vanilla, jadi kolom ke-9 ke atas tetap dijatuhkan.

### 18. Hook kosong dan log yang tidak perlu

- `hooks/hud_update_food.cpp:5-8` (file sudah dihapus): hook `Hud.UpdateFood` hanya memanggil
  fungsi aslinya. Tidak ada efek, tetapi tetap menambah biaya detour setiap frame.
- `hooks/is_wet.cpp:5` (file sudah dihapus): hook `Cover.IsUnderRoof` juga hanya memanggil fungsi aslinya,
  padahal method ini sering dipanggil game (cek atap untuk Wet, api, dan istirahat).
- `hooks/have_empty_slot.cpp:10` (file sudah dihapus): `LOG(VERBOSE)` di setiap pemanggilan.
- [start_attack.cpp:217](../src/hooks/start_attack.cpp#L217): `LOG(FATAL) << "No target found within FOV"`
  ditulis setiap kali aimbot menembak tanpa target. Level FATAL di AsyncLogger tidak menghentikan game, jadi
  ini hanya log berlebih, bukan crash.
- Saran: hapus hook kosong, dan turunkan atau hapus log di jalur yang sering dipanggil.
- **Status**: diperbaiki. Hook `Cover.IsUnderRoof` dan `Inventory.HaveEmptySlot` dihapus beserta filenya.
  `hooks::hud_update_food` ternyata tidak pernah didaftarkan (jadi bukan biaya per frame, hanya kode mati), dan
  ikut dihapus. Log FATAL "No target found within FOV" dihapus.

### 19. Unload tidak mematikan fitur looped

- Lokasi: [commands.cpp:108](../src/commands/commands.cpp#L108) dan
  [main.cpp:117-123](../src/main.cpp#L117-L123).
- Penyebab: `commands::shutdown()` memanggil `on_disable` untuk setiap fitur looped yang menyala, tetapi fungsi
  ini tidak dipanggil di mana pun. Saat unload, `main.cpp` hanya mematikan crafting, animal, world time,
  kamera, dan HUD, lalu mematikan hook.
- Akibat: kalau mod di-unload saat fitur looped masih menyala, nilai yang diubahnya tetap ada di game sampai
  karakter atau world dimuat ulang. Contohnya base HP, stamina, carry weight, eitr, regen, nilai kapal,
  peluang raid yang tetap 0, dan ukuran inventory. Karena hook `drop_invalid_items` ikut dimatikan, item di
  luar grid vanilla juga bisa dijatuhkan saat spawn berikutnya (lihat bug 17).
- Saran: panggil `commands::shutdown()` di thread game sebelum `g_hooking->disable()`, misalnya lewat task
  terakhir seperti pola `world_time::shutdown`.
- **Status**: diperbaiki. `main.cpp` menjalankan `commands::shutdown()` lewat `async_run`, lalu menunggu
  maksimal 2 detik sebelum shutdown lain dan sebelum hook dimatikan. Penanda selesai dikirim sebagai task baru
  setelah `commands::shutdown()`, dan antrean task berjalan berurutan, jadi pemulihan yang ditunda di dalam
  `on_disable` (kapal, raid, eitr, fishing) ikut selesai sebelum unload berlanjut. Saat itu `g_running` sudah
  false, padahal `game_update` hanya men-tick `task_mgr` selama `g_running` atau ada cleanup yang tertunda, jadi
  `g_shutdown_pending` dinyalakan selama menunggu supaya task ini tetap berjalan.

### 20. Toggle fitur dan beberapa tombol menu memanggil API Unity dari thread render

- Lokasi: [gui.cpp:84-88](../src/gui.cpp#L84-L88) (menu diproses di callback DirectX),
  [bool_command.cpp:34-40](../src/commands/bool_command.cpp#L34-L40), dan `bool_option::handle_action`
  di Astra UI.
- Penyebab: input menu diproses di dalam hook Present. Toggle fitur memanggil `set_state`, yang langsung
  menjalankan `on_enable` atau `on_disable` di thread itu. Sembilan dari 139 tombol menu juga memanggil kode
  game tanpa `queue_job` atau `async_run`, misalnya "Set Tailwind from Boat Heading"
  ([base_tools_menu.cpp:204](../src/menu/submenu/base_tools_menu.cpp#L204)), "Copy Current Coordinates"
  ([player_menu.cpp:1303](../src/menu/submenu/player_menu.cpp#L1303)), dan "Add Teleport"
  ([teleport_submenu.cpp:364](../src/menu/submenu/teleport_submenu.cpp#L364)).
- Akibat: di Unity untuk Windows, Present biasanya dipanggil dari thread render, bukan thread utama. API Unity
  yang hanya boleh di thread utama (transform, RectTransform, dan sejenisnya) bisa gagal, dan karena
  exception ditelan (bug 22), kegagalannya tidak terlihat. Contohnya `on_disable` Inventory Size memanggil
  `InventoryGui.SetInventorySize`, yang mengubah `RectTransform.sizeDelta`. Penulisan field langsung juga
  berlomba dengan thread game. Belum dipastikan di dalam game apakah Valheim memakai thread render terpisah.
- Saran: jalankan `on_enable`, `on_disable`, dan aksi tombol lewat `queue_job` ke thread game, seperti yang
  sudah dilakukan 130 tombol lainnya.
- **Status**: diperbaiki.
  - `bool_command::set_state` sekarang menjalankan `on_enable`/`on_disable` lewat `async_run`, setelah
    `m_state` diganti.
  - Tombol yang memanggil Mono langsung sekarang membaca data game di thread game: Set Tailwind from Boat
    Heading, Copy Current Coordinates, Add Map Pin Here, Add Category, dan Add Teleport.
  - Hasilnya dikirim balik ke thread render lewat `render_queue` baru, yang dijalankan sekali per frame
    sebagai dx callback pertama. Ini dipakai untuk clipboard dan untuk menyimpan lokasi teleport, karena daftar
    lokasi dibaca menu setiap frame. Arah tailwind dikirim lewat `std::atomic`.
  - Add Teleport sekarang menyimpan ke kategori submenu yang sedang dibuka, bukan ke variabel global
    `category` (bagian dari #51).
  - Tombol menu Cheat Status tidak disentuh.

### 21. Prevent Freezing/Cold/Wet dan Auto-Purge bertarung dengan game setiap frame

- Lokasi: [player_update.cpp:257-265](../src/hooks/player_update.cpp#L257-L265),
  [buffs.cpp:9-25](../src/features/buffs.cpp#L9-L25), dan daftar debuff di
  [buff_tools.cpp:16](../src/unity/buff_tools.cpp#L16).
- Penyebab: mod menghapus efek setiap frame, sementara `Player.UpdateEnvStatusEffects` menambahkannya lagi
  setiap tick selama kondisinya masih berlaku (hujan, malam dingin, pegunungan). Setiap penambahan meng-clone
  objek `StatusEffect` dan menjalankan `Setup`, yang memunculkan efek awal (partikel atau suara). Auto-Purge
  juga menghapus `Encumbered`, yang ditambahkan lagi setiap tick selama beban berlebih, dan `SoftDeath`, yang
  hanya ikon karena perlindungan skill dihitung dari timer `HardDeath()`.
- Akibat: ikon efek berkedip, dan selama kondisinya berlaku game terus membuat dan membuang objek efek setiap
  tick.
- Saran: cegah efek ditambahkan (hook `SEMan.AddStatusEffect` untuk hash tertentu milik pemain lokal), bukan
  menghapusnya setelah ada. Keluarkan `SoftDeath` dan `Encumbered` dari daftar Auto-Purge.
- **Status**: diperbaiki.
  - Hook baru [add_status_effect.cpp](../src/hooks/add_status_effect.cpp) dipasang pada
    `SEMan.AddStatusEffect(StatusEffect, …)`. Overload berbasis hash juga berakhir di sini lewat
    `Internal_AddStatusEffect`, jadi semua sumber efek tertangkap.
  - Hook mengembalikan null untuk `SEMan` milik pemain lokal kalau hash-nya Freezing, Cold, atau Wet (sesuai
    setting) atau termasuk debuff saat Auto-Purge menyala. Hash dibaca dari `StatusEffect.m_nameHash`, dan
    `NameHash()` hanya dipanggil kalau nilainya masih 0.
  - Penghapusan per frame di `player_update` sekarang hanya terjadi sekali saat opsinya dinyalakan.
  - Auto-Purge menjadi `bool_command` yang membersihkan debuff sekali di `on_enable`.
  - `Encumbered` dan `SoftDeath` dikeluarkan dari daftar debuff.

### 22. Exception Mono ditelan tanpa log

- Lokasi: [mono.cpp:143-164](../src/services/mono/mono.cpp#L143-L164).
- Penyebab: `invoke_method_impl` selalu menangkap exception managed, lalu mengembalikan `nullptr`. Kalau
  pemanggil tidak meminta parameter exception, tidak ada yang mencatatnya.
- Akibat: kegagalan seperti di bug 13 dan bug 20 tidak meninggalkan jejak di log, sehingga sulit dilacak.
- Saran: catat exception yang tidak diminta pemanggil, dengan batas jumlah log per method supaya tidak
  membanjiri.
- **Status**: diperbaiki. Kalau pemanggil tidak meminta parameter exception, `invoke_method_impl` mencatat
  class dan nama method, tipe exception, dan isi `Exception._message`, maksimal 3 kali per method.

### 23. Jendela ImGui menjalankan aksi game dari thread render

- Lokasi: [equipment_slots.cpp:1994-2004](../src/unity/equipment_slots.cpp#L1994-L2004),
  [bookmark_manager.cpp:169-206](../src/unity/bookmark_manager.cpp#L169-L206), dan
  [gui_binding.cpp:178-222](../src/services/javascript/bindings/gui_binding.cpp#L178-L222). Tombol Instant Tame
  dan Heal Full di overlay Animal Inspector ([animal_tools.cpp:658-669](../src/unity/animal_tools.cpp#L658-L669))
  juga memanggil `tame_aimed_creature` dan `heal_aimed_creature` langsung dari callback gambar.
- Penyebab: semua jendela ImGui digambar di callback DirectX, sama seperti menu di bug 20. Tombol di dalamnya
  langsung menjalankan kode game tanpa `queue_job`:
  - Equipment & Quick Slots: Auto-Fill, Return All, dan Save Loadout memanggil `Inventory.MoveItemToThis`,
    `Humanoid.EquipItem`, `UnequipAllItems`, dan `save_to_player`.
  - Bookmark: Save Current Location membaca posisi pemain lalu menulis `Player.m_customData`, dan tombol Delete
    juga menulis dictionary itu. Daftar bookmark dibaca lewat `Dictionary.TryGetValue` setiap frame (P15).
    Tombol Teleport sudah lewat `async_run`, jadi berjalan di thread game.
  - Opsi menu dari skrip JS memanggil callback JS langsung, jadi skrip yang memakai binding Mono ikut berjalan
    di thread itu.
- Akibat: kalau thread render memang terpisah dari thread game, pemindahan item berjalan bersamaan dengan
  game yang juga membaca atau mengubah inventory yang sama. Ini berisiko item ganda, item hilang, atau crash.
  `EquipItem` juga mengganti model visual karakter, yang termasuk API Unity untuk thread utama. Kegagalannya
  tidak terlihat karena bug 22.
- Saran: bungkus aksi tombol di jendela-jendela ini dengan `queue_job`, seperti yang sudah dilakukan
  `global_key_editor` dan `status_effect_browser` untuk aksinya.
- **Status**: diperbaiki, kecuali opsi menu dari skrip JS.
  - Equipment: Auto-Fill, Return All, Save Loadout, klik slot, klik kanan slot, dan pilihan di picker sekarang
    berjalan lewat `async_run`.
  - Bookmark: Save dan Delete juga lewat `async_run`.
  - Animal Inspector: Instant Tame dan Heal Full juga lewat `async_run`.
  - Callback JS sengaja tidak dipindah. QuickJS dipakai dari thread render untuk `eval` dan JS GUI, jadi
    memindahkan callback ke thread game akan membuat QuickJS lebih sering dipakai dari dua thread, dan QuickJS
    tidak thread-safe. Ini perlu diselesaikan bersama #48 dengan satu thread untuk semua JS.
  - Catatan terkait #20: `on_enable`/`on_disable` dari command buatan skrip JS sekarang ikut berjalan di thread
    game, sama seperti `on_tick` yang memang sudah di sana.

### 24. Hook aimbot tidak mengecek penembak

- Lokasi: [start_attack.cpp:187](../src/hooks/start_attack.cpp#L187) (`Attack.GetProjectileSpawnPoint`) dan
  [on_hit.cpp:92](../src/hooks/on_hit.cpp#L92) (`Projectile.OnHit`).
- Penyebab: kedua hook hanya mengecek apakah aimbot menyala, tidak mengecek apakah serangan atau proyektilnya
  milik pemain lokal.
- Akibat: selama aimbot menyala, serangan jarak jauh creature yang dikontrol client Anda (misalnya pemanah
  Fuling) ikut diarahkan ke target terdekat dari crosshair Anda, dan titik kena proyektil siapa pun yang
  diproses client Anda dipindah ke kepala target itu, termasuk posisi efek area. Selain itu `find_best_target`
  memindai semua karakter dengan sekitar 4 panggilan Mono plus `Camera.main` per karakter, untuk setiap
  proyektil yang ditembakkan dan setiap proyektil yang mengenai sesuatu.
- Saran: cek `Attack.m_character` (dan pemilik proyektil di `OnHit`) sama dengan pemain lokal sebelum
  mengubah apa pun.
- **Status**: diperbaiki. `GetProjectileSpawnPoint` sekarang mengecek `Attack.m_character`, dan `OnHit`
  mengecek `Projectile.m_owner`. Keduanya hanya mengubah arah atau titik kena kalau penembaknya pemain lokal.
  `find_best_target` juga tidak lagi dijalankan untuk proyektil milik karakter lain.

### 25. Item di slot equipment tidak pernah masuk tombstone

- Lokasi: [equipment_slots.cpp:337-438](../src/unity/equipment_slots.cpp#L337-L438) (memuat halaman dari
  `m_customData`) dan [equipment_slots.cpp:512-656](../src/unity/equipment_slots.cpp#L512-L656) (menyimpan).
- Penyebab: item di slot equipment disimpan di inventory terpisah yang ditulis ke `Player.m_customData`.
  `Player.CreateTombStone` hanya memindahkan `Humanoid.m_inventory` ke tombstone (`MoveInventoryToGrave`), dan
  mod tidak menangani kematian.
- Akibat: armor, utility, makanan, ammo, dan quick slot di slot equipment tidak pernah jatuh saat mati,
  walaupun "Don't Drop Items when Dead" mati, dan fitur ini menyala secara default. Sebaliknya, kalau karakter
  dimuat tanpa mod, semua item itu tidak terlihat sama sekali (hanya ada di `m_customData`), jadi karakter
  muncul tanpa armor sampai mod dipasang lagi.
- Saran: saat `CreateTombStone`, pindahkan isi halaman equipment ke tombstone kecuali "Don't Drop Items when
  Dead" menyala. Tulis juga di dokumentasi bahwa item di slot equipment bergantung pada mod.
- **Status**: perilaku dipertahankan atas keputusan pemilik: item di slot equipment memang selalu aman saat mati.
  Deskripsi opsi Equipment Slots sekarang menyebut bahwa item di slot ini tidak pernah jatuh saat mati dan hanya
  terlihat selama mod aktif.

### 26. Return All melaporkan berhasil walaupun inventory penuh

- Lokasi: [equipment_slots.cpp:731-774](../src/unity/equipment_slots.cpp#L731-L774), dipakai
  [equipment_slots.cpp:945-972](../src/unity/equipment_slots.cpp#L945-L972).
- Penyebab: `move_from_slot_to_inventory` melepas item (UnequipItem) lebih dulu, lalu memanggil
  `MoveItemToThis` dan mengabaikan hasilnya. Fungsi ini selalu mengembalikan `true`.
- Akibat: kalau inventory utama penuh, item tetap di slot tetapi armor sudah terlepas dari karakter, dan
  notifikasi "Deposited N items" menghitung item yang sebenarnya tidak pindah.
- Saran: cek hasil `MoveItemToThis` sebelum unequip, dan kembalikan `false` kalau gagal.
- **Status**: diperbaiki. `MoveItemToThis(Inventory, ItemData)` mengembalikan `void`, jadi sebelum unequip
  fungsi ini sekarang mengecek `Inventory.CanAddItem(item, -1)` pada inventory utama. Setelah dipindah, fungsi
  mengecek lagi lewat `ContainsItem` bahwa item sudah tidak ada di slot. Kalau gagal, fungsi mengembalikan
  `false`, armor tidak dilepas, dan item tidak dihitung di "Deposited N items".

### 27. Jalur exception SEH di `drop_item` dan `parry_hooks`

- Lokasi: [drop_item.cpp:11-40](../src/hooks/drop_item.cpp#L11-L40) dan
  [parry_hooks.cpp:149-171](../src/hooks/parry_hooks.cpp#L149-L171).
- Penyebab: `EXCEPT_CLAUSE` berisi handler kosong.
  - `drop_item` mengembalikan `MonoObject*`, tetapi setelah blok `EXCEPT_CLAUSE` tidak ada `return`.
  - Di `character_rpc_damage`, kalau exception terjadi di dalam blok auto parry, eksekusi lanjut ke pemanggilan
    fungsi asli di akhir fungsi.
- Akibat: kalau exception SEH terjadi (jarang), `drop_item` mengembalikan pointer sampah ke game sebagai
  `ItemDrop`, yang hampir pasti membuat crash. Di parry, satu serangan bisa diproses dua kali.
- Saran: tambahkan `return nullptr;` setelah blok di `drop_item`, dan `return` di dalam handler parry.
- **Status**: parry diperbaiki: setelah exception di blok auto parry, fungsi sekarang `return` dan tidak lagi
  memanggil fungsi asli untuk kedua kalinya. Bagian `drop_item` tidak dikerjakan, karena hook itu juga
  menangani flag `m_cheated` pada item, dan tooling seperti itu berada di luar cakupan perbaikan ini.

### 28. Makanan di slot tambahan tidak dihitung ke stat makanan

- Lokasi: [eat_food.cpp:230-310](../src/hooks/eat_food.cpp#L230-L310).
- Penyebab: saat Max Food Slots lebih dari 3, makanan keempat dan seterusnya ditambahkan langsung ke
  `m_foods`, tanpa memanggil `Player.EatFood` asli.
- Akibat: stat "makanan dimakan" (`IncrementStatFoodEaten`) dan efek lain di `EatFood` asli tidak berjalan
  untuk makanan di slot tambahan.
- Saran: panggil `PlayerProfile.IncrementStatFoodEaten` di jalur ini, atau panggil `EatFood` asli setelah
  memberi ruang di `m_foods`.
- **Status**: diperbaiki. Jalur slot tambahan sekarang memanggil hal yang sama seperti `EatFood` vanilla:
  `Game.IncrementPlayerStat(FoodEaten = 49, 1, false)` dan `PlayerProfile.IncrementStatFoodEaten(m_name, 1,
  item.m_cheated)`.

### 29. Remote Trader memasang animator karakter ke prefab trader

- Lokasi: [world_tools.cpp:483-487](../src/unity/world_tools.cpp#L483-L487) (jarak tutup toko),
  [world_tools.cpp:502-526](../src/unity/world_tools.cpp#L502-L526) (pencocokan nama), dan
  [world_tools.cpp:551-576](../src/unity/world_tools.cpp#L551-L576) (animator).
- Penyebab:
  - Kalau trader tidak ada di sekitar, mod memakai komponen `Trader` dari prefab di `ZNetScene`. `m_animator`
    prefab itu kosong karena `Trader.Start` tidak pernah berjalan di prefab, jadi mod mengisinya dengan
    `Animator` karakter Anda. Pengecekan `!cur_anim` hanya melihat pointer managed, jadi referensi ini tidak
    pernah diganti lagi.
  - `Trader.Say` memanggil `m_animator.SetTrigger("Buy")` atau `SetTrigger("Sell")` di setiap transaksi, dan
    trigger itu masuk ke animator karakter Anda.
  - Nama trader dicocokkan dengan `find("Haldor")` pada `Trader.m_name`. File lokalisasi game memakai kunci
    `npc_haldor`, `npc_hildir`, dan `npc_bogwitch`, jadi `m_name` kemungkinan besar `$npc_haldor` dan
    seterusnya. Pencarian peka huruf besar-kecil, jadi nama tidak pernah cocok, dan loop memakai trader pertama
    yang ditemukan.
  - `StoreGui.m_hideDistance` diubah ke 99999999 dan tidak pernah dikembalikan.
- Akibat:
  - Setelah karakter mati atau ganti world, animator lama sudah dihancurkan tetapi tetap tersimpan di prefab.
    Setiap beli atau jual lewat prefab lalu melempar exception di `OnBought` atau `OnSold`, setelah item dan
    koin sudah pindah. Efek beli, pesan pickup, dan `FillList` tidak berjalan, jadi daftar toko tidak diperbarui.
  - Sebelum mati, trigger `Buy` dan `Sell` dikirim ke animator karakter Anda. Kalau controller pemain tidak
    punya parameter itu, akibatnya hanya warning di log. Ini belum dipastikan, karena isi controller ada di asset.
  - Kalau ada trader lain yang sedang dimuat (misalnya Anda di dekat Hildir lalu memilih Haldor), toko yang
    terbuka adalah trader itu.
  - Sampai keluar world, toko trader biasa tidak lagi tertutup saat Anda berjalan menjauh.
- Saran: jangan menulis `m_animator` ke prefab. Kalau tetap perlu, cek objeknya lewat
  `UnityEngine.Object.op_Implicit`, bukan pointer. Cocokkan trader lewat nama `GameObject` (prefab `Haldor`,
  `Hildir`, `BogWitch`) atau kunci `$npc_` dengan huruf kecil. Simpan `m_hideDistance` asli dan kembalikan
  saat toko ditutup.
- **Status**: diperbaiki.
  - Trader dicocokkan lewat nama `GameObject` yang diawali nama prefab (`Haldor`, `Hildir`, `BogWitch`).
    Fallback ke "trader pertama yang ditemukan" dihapus. Kalau tidak ada yang cocok, mod memakai prefab.
  - Animator karakter pemain tidak lagi dipasang. Kalau `m_animator` kosong atau objeknya sudah dihancurkan
    (dicek lewat `UnityEngine.Object.op_Implicit`), mod mengisinya dengan animator milik trader itu sendiri lewat
    `GetComponentInChildren(Animator, includeInactive: true)`.
  - `m_hideDistance` asli disimpan, lalu sebuah task menunggu sampai `StoreGui.IsVisible()` bernilai false dan
    mengembalikan nilainya.

### 30. Allow Picking Up Fish mengubah fisika dan gerak semua ikan

- Lokasi: `hooks/is_out_of_water.cpp:8` (file sudah dihapus), hook `Fish.IsOutOfWater`.
- Penyebab: hook mengembalikan `true` untuk semua ikan selama setting menyala. Fungsi ini tidak hanya dipakai
  `Interact` dan `GetHoverText`, tetapi juga oleh fisika ikan di `Fish.CustomFixedUpdate`, `ConsiderJump`, dan
  `Jump`.
- Akibat, untuk ikan yang dikontrol client Anda:
  - Ikan yang menyentuh dasar atau batu melompat dengan gaya lompat darat dan putaran acak.
  - Ikan yang badannya menyentuh permukaan berhenti berenang dan ditarik gravitasi sampai tenggelam lagi.
  - Gaya ombak dan angin pada ikan tidak diterapkan.
  - Efek percikan ikan yang tersangkut kail tidak muncul.
  - Posisi ikan ini disinkronkan, jadi pemain lain juga melihat gerakan yang sama.
- Saran: biarkan `IsOutOfWater` asli. Hook `Fish.Interact` dan `Fish.GetHoverText`, lalu panggil `Pickup`
  langsung saat setting menyala.
- **Status**: diperbaiki sesuai saran. Hook `IsOutOfWater` dihapus. [fish_pickup.cpp](../src/hooks/fish_pickup.cpp)
  memasang hook `Fish.Interact`, yang memanggil `Fish.Pickup` langsung kecuali saat tombol ditahan, dan hook
  `Fish.GetHoverText`, yang memakai teks pickup yang sama dengan vanilla saat ikan di luar air. Fisika ikan
  tidak lagi terpengaruh.

### 31. No Weight baru berlaku setelah isi inventory berubah

- Lokasi: [get_weight.cpp:9](../src/hooks/get_weight.cpp#L9), hook `ItemDrop.ItemData.GetWeight`.
- Penyebab:
  - `Inventory.GetTotalWeight` hanya mengembalikan `m_totalWeight`. Nilai itu dihitung ulang di
    `Inventory.Changed`, yaitu saat item masuk, keluar, atau dipindah.
  - Hook berlaku untuk item di semua inventory, bukan hanya inventory pemain.
- Akibat:
  - Saat No Weight dinyalakan, bobot dan status encumbered tetap memakai nilai lama sampai Anda mengambil atau
    memindah item. Saat dimatikan, bobot tetap 0 dan karakter tidak encumbered walaupun membawa beban penuh,
    sampai isi inventory berubah. Override `ew_weight` per item punya masalah yang sama.
  - Selama menyala, isi gerobak juga dihitung 0, karena `Vagon.UpdateMass` memakai total bobot container.
    Gerobak yang dikontrol client Anda jadi seringan gerobak kosong.
- Saran: batasi ke inventory pemain lokal, misalnya lewat hook `Inventory.UpdateTotalWeight` yang mengecek
  `this`. Panggil `Inventory.Changed` milik pemain (private, tetapi bisa dipanggil lewat Mono) saat setting
  berubah supaya total langsung dihitung ulang.

### 32. Builder submenu berjalan setiap frame

- Lokasi: builder di [teleport_submenu.cpp:243-410](../src/menu/submenu/teleport_submenu.cpp#L243-L410),
  [player_menu.cpp:1797-1850](../src/menu/submenu/player_menu.cpp#L1797-L1850), dan
  [player_menu.cpp:1888-1905](../src/menu/submenu/player_menu.cpp#L1888-L1905). Di repo astra-ui:
  `host/canvas.cpp:217-218` dan `add_submenu_impl` di `include/astra/host/canvas.hpp:144-150`.
- Penyebab: setiap `canvas::tick`, Astra memanggil `reset()` lalu `execute()` pada submenu yang sedang aktif.
  Jadi lambda builder submenu berjalan setiap frame di callback gambar (thread render, lihat bug 20). Builder
  yang hanya menambah opsi tidak masalah, tetapi empat builder ini membaca game atau file:
  - Custom Teleport membaca dan mem-parse `teleport_location.json` setiap frame, lalu memanggil
    `canvas::add_submenu` untuk setiap kategori. `add_submenu_impl` selalu membuat submenu baru dan
    menambahkannya ke `m_all_submenu` tanpa cek ID, dan vector itu tidak pernah dikecilkan.
  - Guardian Powers dan Individual Boss Powers memanggil `buff_tools::get_boss_powers()`, yang menelusuri
    semua `ObjectDB.m_StatusEffects` dan memanggil `get_name` untuk setiap efek. Hasilnya disimpan di satu
    vector `static` yang dikosongkan lalu diisi ulang setiap kali dipanggil. `discover_guardian_powers` dan
    `activate_all_guardian_powers` di thread game memakai vector yang sama
    ([buff_tools.cpp:358](../src/unity/buff_tools.cpp#L358), [buff_tools.cpp:424](../src/unity/buff_tools.cpp#L424)),
    dan builder menyimpan pointer `c_str()` ke string di dalamnya untuk pilihan "Selected Guardian Power".
  - Base Raids & Events menelusuri semua `RandomEvent` beserta daftar spawn-nya lewat Mono.
  - Objectives dan Map Waypoints membaca semua pin minimap lewat Mono, satu panggilan `get_Item` per pin.
- Akibat:
  - Selama Custom Teleport terbuka, memori terus bertambah: satu objek submenu per kategori per frame (sekitar
    180 objek per detik untuk 3 kategori di 60 FPS), dan tidak dibebaskan sampai mod di-unload. Ditambah baca
    file dan parse JSON setiap frame.
  - Menekan Discover Guardian Powers atau Apply All Registered Guardian Effects membuat thread game
    mengosongkan vector yang mungkin sedang dibaca thread render. Ini bisa berujung crash atau nama pilihan
    yang rusak. Peluangnya kecil per klik, tetapi ada.
  - Halaman-halaman ini menurunkan FPS selama terbuka, karena ratusan panggilan Mono per frame dari thread
    render.
- Saran: siapkan datanya di thread game (seperti snapshot Base Tools), lalu biarkan builder hanya membaca
  salinannya. Muat file teleport sekali saat menu dibuka atau setelah disimpan. Daftarkan submenu kategori
  sekali, atau buat `add_submenu_impl` di Astra mengganti submenu yang ID-nya sama. Buat `get_boss_powers`
  mengembalikan salinan, bukan referensi ke vector `static`.

### 33. `ItemData.m_worldLevel` dibaca dan ditulis sebagai 1 byte

- Lokasi: [item_spawner.cpp:620-623](../src/unity/item_spawner.cpp#L620-L623),
  [item_spawner.cpp:1037-1038](../src/unity/item_spawner.cpp#L1037-L1038),
  [inventory_editor.cpp:221](../src/unity/inventory_editor.cpp#L221), dan
  [inventory_editor.cpp:654-666](../src/unity/inventory_editor.cpp#L654-L666).
- Penyebab: di build sekarang field ini bertipe `System.Int32` (dicek lewat runtime Mono game), tetapi mod
  memakai `uint8_t`. `mono_field_get_value` menyalin 4 byte ke variabel 1 byte, dan `mono_field_set_value`
  membaca 4 byte dari variabel 1 byte.
- Akibat: saat membaca, 3 byte di stack sebelah variabel ikut tertimpa. Saat Spawn in World menulis, 3 byte
  sisa di stack ikut masuk ke world level item. Game memotong nilai ini ke byte saat item dimuat dari ZDO atau
  save, jadi item yang sudah diambil biasanya kembali benar, tetapi selama masih di tanah nilainya bisa salah.
  Ini undefined behavior, jadi akibat persisnya bergantung pada susunan stack hasil compile.
- Saran: pakai `int` di semua tempat itu.

### 34. Tiga lookup memakai nama field yang sudah tidak ada di game

- Lokasi: [procreation.cpp:34-50](../src/unity/procreation.cpp#L34-L50),
  [tameable.cpp:85](../src/unity/tameable.cpp#L85), [tameable.cpp:220](../src/unity/tameable.cpp#L220), dan
  [unity.hpp:42-55](../src/utility/unity.hpp#L42-L55).
- Penyebab dan akibat:
  - `Procreation.m_lovePoints` tidak ada. Love points disimpan di ZDO dan dibaca lewat
    `Procreation.GetLovePoints()`. Akibatnya `get_love_points` selalu 0, dan baris "Breeding: Ready (love x / y)"
    di inspector hewan ([animal_tools.cpp:643-645](../src/unity/animal_tools.cpp#L643-L645)) tidak pernah muncul.
  - `Tameable.m_tameTimeLeft` tidak ada, jadi `set_tame_time_left` tidak melakukan apa-apa. Fitur tame tetap
    jalan karena `Tame()` dipanggil sesudahnya.
  - `unity::get_zone_system()` hanya mencari `ZoneSystem.m_instance`, padahal sekarang namanya `s_instance`,
    jadi fungsi ini selalu mengembalikan `nullptr`. `zone_system::get_instance()` punya fallback dan aman,
    tetapi `getZoneSystem` di binding JS selalu 0, dan log Item Spawner selalu menulis `ZoneSystem: false`.
- Saran: panggil `GetLovePoints()`, hapus `set_tame_time_left`, dan cari `s_instance` lebih dulu di
  `get_zone_system()`.

### 35. Masalah kecil di Prefab Spawner, Creature Spawner, dan custom teleport

- Lokasi: [prefab_spawner.cpp:447-477](../src/unity/prefab_spawner.cpp#L447-L477),
  [prefab_spawner.cpp:552-560](../src/unity/prefab_spawner.cpp#L552-L560),
  [creature_spawner.cpp:319-326](../src/unity/creature_spawner.cpp#L319-L326), dan
  [custom_teleport_service.cpp:106-113](../src/services/custom_teleport/custom_teleport_service.cpp#L106-L113).
- Prefab Spawner:
  - Posisi spawn diisi sekali per sesi mod dan tidak pernah diperbarui. Setelah teleport atau ganti world,
    nilai default masih menunjuk ke tempat lama, jadi spawn tanpa mengubah angka menaruh objek di lokasi itu.
    Pengisian pertama juga membaca posisi lewat Mono dari thread render.
  - Undo Last selalu menampilkan "Removed" walaupun objeknya sudah hilang (misalnya creature sudah mati),
    karena exception dari `ZNetScene.Destroy` ditelan.
- Creature Spawner: Spawn tamed memanggil `set_tamed(true)` sebelum `Tameable.Tame()`. `Tame()` lalu melewatkan
  efek dan pesan karena creature sudah jinak, tetapi tetap menambah stat profil `CreatureTamed` untuk setiap
  creature yang di-spawn.
- `custom_teleport_service::get_saved_location_by_name` mengembalikan pointer ke elemen vector sementara yang
  langsung dihancurkan. Fungsi ini belum dipakai, tetapi akan membaca memori yang sudah dibebaskan kalau nanti
  dipakai.
- Saran: isi ulang posisi Prefab Spawner saat jendela dibuka (dari thread game), cek hasil `Destroy`, cukup
  panggil `Tame()` tanpa `set_tamed` lebih dulu, dan kembalikan salinan dari `get_saved_location_by_name`.

### 36. Grid equipment native kemungkinan hanya menampilkan item pertama (belum dites di game)

- Lokasi: [equipment_slots.cpp:1612-1632](../src/unity/equipment_slots.cpp#L1612-L1632) (delegate yang
  disalin) dan [equipment_slots.cpp:1955-1960](../src/unity/equipment_slots.cpp#L1955-L1960) (`UpdateInventory`
  setiap frame).
- Penyebab: grid equipment dibuat dengan `Object.Instantiate` dari grid inventory pemain. Unity tidak menyalin
  field delegate saat instantiate, jadi mod menyalin `m_onSelected` dan `m_onRightClick` secara manual. Di build
  sekarang, `InventoryGui.Awake` memasang delegate lain juga ke grid pemain: `CanDropDragOntoItem`,
  `m_onReleased`, `m_onEnter`, `OnSetTouchSelection`, `OnMoveToUpperInventoryGrid`, dan
  `OnMoveToLowerInventoryGrid`. Di IL `InventoryGrid.UpdateGui`, setiap item menjalankan
  `element.m_canBeDroppedOn = CanDropDragOntoItem.Invoke(item)` tanpa cek null.
- Akibat (dari IL, belum dites di game):
  - `UpdateGui` di grid equipment melempar NullReferenceException pada item pertama setiap frame. Item-item
    berikutnya tidak pernah digambar. Loop di akhir fungsi yang menyembunyikan ikon sel kosong juga tidak
    pernah jalan, jadi ikon lama bisa tertinggal.
  - Exception ini ditelan (bug 22), tetapi tetap dibuat setiap frame selama inventory terbuka, dan membuat
    exception di Mono termasuk mahal.
  - `m_onReleased` yang kosong membuat drag-and-drop layar sentuh ke grid ini tidak bekerja.
  - Salinan grid juga membawa elemen sel milik grid pemain (anak `m_gridRoot`), karena `m_elements` bersifat
    private dan tidak ikut disalin. Elemen salinan itu tidak dilacak dan tidak pernah dihapus. Kalau grid dibuat
    saat grid pemain sudah punya elemen (misalnya mod di-inject setelah inventory pernah dibuka), sel bertipe
    `none` bisa menampilkan salinan beku sel inventory pemain. Ini juga belum dipastikan.
- Cara cek cepat: taruh dua item atau lebih di slot equipment, buka inventory, lalu lihat apakah semuanya
  muncul.
- Saran: di `create_native_grid`, salin juga `CanDropDragOntoItem`, `m_onReleased`, `m_onEnter`,
  `OnSetTouchSelection`, dan dua `OnMoveTo...` dari `m_playerGrid`, lalu hapus anak `m_gridRoot` hasil salinan
  sebelum `UpdateInventory` pertama.

### 37. Unload tidak menghapus grid equipment

- Lokasi: [main.cpp:117-123](../src/main.cpp#L117-L123),
  [equipment_slots.cpp:270-290](../src/unity/equipment_slots.cpp#L270-L290), dan
  [equipment_slots.cpp:1426-1450](../src/unity/equipment_slots.cpp#L1426-L1450).
- Penyebab: unload tidak memanggil `destroy_native_ui` atau `release_handles` di thread game. Pembersihan
  hanya ada di destructor `static` milik `equipment_manager`, yang berjalan saat `FreeLibraryAndExitThread` di
  thread mod sambil memegang loader lock. Di sana `Component.get_gameObject` dan `Object.Destroy` dipanggil dari
  thread selain main thread, jadi Unity menolaknya (exception-nya ditelan) dan grid tetap ada.
- Akibat:
  - Kalau unload terjadi saat inventory terbuka, grid equipment tetap terlihat dan masih bisa diklik, karena
    delegate-nya menunjuk ke `InventoryGui` milik game. Gambarnya tidak diperbarui lagi.
  - Item yang ditaruh ke grid itu setelah unload tidak lagi ditulis ke `m_customData`, jadi hilang saat keluar.
  - Item yang diambil dari grid itu masuk ke inventory utama, tetapi salinan lamanya masih ada di
    `m_customData`. Saat mod di-inject lagi, `init_or_sync` memuat halaman equipment tanpa mengecek inventory
    utama, jadi item itu muncul dua kali.
  - Kalau inventory tertutup saat unload, grid tetap ada tetapi nonaktif, jadi tidak terlihat.
  - Memanggil Mono dari destructor saat loader lock dipegang juga berisiko hang saat unload.
- Saran: panggil `equipment_manager::get().release_handles()` lewat task di thread game sebelum
  `g_hooking->disable()`, seperti pola `world_time::shutdown`, dan buang panggilan Mono dari destructor.

### 38. Tame All in Radius dan Tame Aimed Creature ikut mengambil alih karakter pemain lain

- Lokasi: [character.cpp:184-187](../src/unity/character.cpp#L184-L187),
  [animal_tools.cpp:90](../src/unity/animal_tools.cpp#L90),
  [animal_tools.cpp:343](../src/unity/animal_tools.cpp#L343), dan
  [animal_tools.cpp:154-205](../src/unity/animal_tools.cpp#L154-L205).
- Penyebab: `character::is_player()` selalu mengembalikan `false`. Override yang benar hanya ada di wrapper
  `player`, padahal `get_all_characters()` dan target inspector membuat wrapper `character`. Jadi pengecekan
  `is_player()` di `animal_tools` tidak pernah mengecualikan pemain lain (pemain lokal tetap dikecualikan lewat
  perbandingan pointer).
- Akibat:
  - Tame All in Radius (10 sampai 100 m) dan Tame Aimed Creature (termasuk lewat hotkey) menjalankan
    `tame_creature` pada pemain lain yang ada di jangkauan atau di crosshair. Fungsi itu memanggil
    `ZNetView.ClaimOwnership()`, sehingga ZDO karakter pemain itu berpindah pemilik ke client Anda. Setelah itu
    `SetTamed(true)` dijalankan, dan kalau Heal on Tame menyala, HP-nya ikut diubah. Pemain itu kehilangan
    kontrol atas karakternya sendiri sampai kepemilikan kembali.
  - Inspector hewan juga menampilkan data pemain lain sebagai creature.
  - Kill Hostiles dan Freeze AI aman, karena keduanya juga mensyaratkan `MonsterAI`, yang tidak dimiliki
    pemain.
- Saran: di `character::is_player()`, panggil `Character.IsPlayer()` (method ini sudah dipakai `player::is_player`).
  Di `tame_creature`, tolak target yang tidak punya `Tameable` atau `MonsterAI` sebelum `ClaimOwnership`.

### 39. Teleport XYZ, crosshair, dan pin mengabaikan nilai Y

- Lokasi: [unity.hpp:1223-1264](../src/utility/unity.hpp#L1223-L1264).
- Penyebab: `teleport_to_world_point` memakai `Minimap.DebugTeleport(Vector3)` selama minimap ada. Di game,
  fungsi ini hanya memakai X dan Z, lalu mengganti Y dengan tinggi heightmap terrain (`Heightmap.GetHeight`,
  minimal 0). Jalur cadangan yang memakai Y asli (`TeleportTo` dengan Y + 2) hanya dipakai kalau minimap tidak
  ada.
- Akibat:
  - Teleport to XYZ meminta Y, tetapi Y selalu diabaikan. Interior dungeon dan gua (sekitar Y 5000) tidak bisa
    dituju, karena pemain mendarat di terrain di atasnya.
  - Teleport to Crosshair ke atap, lantai atas, atau batu besar (batu adalah objek, bukan terrain) mendaratkan
    pemain di tinggi terrain di bawahnya, jadi bisa berada di dalam batu atau di bawah lantai.
  - Teleport ke Death Marker setelah mati di dalam dungeon mendarat di permukaan, bukan di dekat tombstone.
    Teleport to Last Tombstone di menu Death memakai jalur lain dan tidak terkena.
  - Teleport ke pemain lain di panel Online Players
    ([online_players.cpp:132-146](../src/unity/online_players.cpp#L132-L146)) juga memakai jalur ini, jadi
    kalau pemain itu ada di dalam dungeon, Anda mendarat di permukaan.
  - Bookmark dan Custom Teleport memakai `Player.TeleportTo` langsung, jadi tidak terkena.
- Saran: pakai `Player.TeleportTo` dengan Y asli untuk XYZ, crosshair, dan pin. Biarkan game menyesuaikan
  tinggi lewat `FindFloor` saat teleport jauh.

### 40. Kesalahan kecil di helper

- Lokasi: [item_drop.cpp:174-209](../src/unity/item_drop.cpp#L174-L209) dan
  [unity.hpp:1637-1653](../src/utility/unity.hpp#L1637-L1653).
- `item_drop::can_pickup` dan `item_drop::can_eat` membaca `*reinterpret_cast<bool*>(ret)` langsung dari
  objek boxed, tanpa `object_unbox`. Yang terbaca adalah byte pertama header objek (alamat vtable), bukan hasil
  method. Hasilnya hampir selalu `true`, tetapi di sesi ketika byte itu kebetulan 0 (sekitar 1 dari 32), hasilnya
  selalu `false`, dan Pickup Nearby Drops tidak mengambil apa pun di sesi itu. `can_eat` belum dipakai.
- `discover_bosses_and_traders` memakai pin type 17 dan 18 untuk lokasi Hildir. Di build sekarang 17 adalah
  `Memorial`, dan 18 tidak ada di enum. Fungsi ini belum dipanggil; versi yang dipakai di `exploration.cpp`
  sudah benar (14, 15, 16).
- Saran: pakai `object_unbox` di kedua fungsi itu, dan hapus atau samakan `discover_bosses_and_traders` dengan
  katalog di `exploration.cpp`.

### 41. Consolidate Stacks di Inventory Editor tidak pernah jalan

- Lokasi: [inventory_editor.cpp:265](../src/unity/inventory_editor.cpp#L265).
- Penyebab: lookup memakai `get_method_exact("Inventory", "RemoveItem", {"ItemDrop/ItemData"})`. Runtime Mono
  game menulis tipe nested dengan titik, yaitu `ItemDrop.ItemData`, jadi `strcmp` dan pencocokan `strstr` di
  `get_method_exact` sama-sama gagal. Ini sudah dicek dengan meniru logika pencocokan mod terhadap runtime game.
- Akibat: `remove_item` selalu `nullptr`, jadi tombol Consolidate Stacks selalu menampilkan "Inventory stack
  methods are unavailable." dan tidak mengubah apa pun.
- Saran: pakai `"ItemDrop.ItemData"`, atau `get_method("Inventory", "RemoveItem", 1)` lalu cek tipe parameternya.
  Dari 32 pemanggilan `get_method_exact` di seluruh kode, hanya yang ini yang gagal.

### 42. Masalah kecil di exploration, breeding, dan fishing

- Lokasi: [exploration.cpp:18](../src/unity/exploration.cpp#L18),
  [exploration.cpp:111-130](../src/unity/exploration.cpp#L111-L130),
  [procreation.cpp:72-126](../src/unity/procreation.cpp#L72-L126), dan
  [fishing_tools.cpp:56-79](../src/unity/fishing_tools.cpp#L56-L79).
- Exploration: kandidat lokasi untuk The Queen berisi `Mistlands_DvergrTownEntrance1`, dan
  `discover_location` memproses semua kandidat, bukan berhenti di yang pertama ketemu. File lokalisasi game
  memakai `location_dvergrtown` untuk Infested Mine dan `location_dvergrboss` untuk Infested Citadel, jadi pin
  "The Queen" juga dipasang di Infested Mine terdekat. Daftar asset game di `StreamingAssets/SoftRef/manifest`
  memastikan kedua prefab ada, yaitu `Assets/world/Locations/Mistlands/Mistlands_DvergrBossEntrance1.prefab`
  dan `Mistlands_DvergrTownEntrance1.prefab`. Jadi kandidat pertama sudah benar, dan kandidat keempat yang
  menambah pin di mine. Belum dites di game.
- Breeding Speed: `Procreation.m_updateInterval` hanya dibaca game di `Awake` untuk `InvokeRepeating`, jadi
  mengubahnya sesudahnya tidak mengubah interval cek breeding. Yang benar-benar berubah hanya
  `m_pregnancyDuration`. Nilai asli juga disimpan di map `static` dengan key pointer mentah yang tidak pernah
  dibersihkan. Setelah creature dihancurkan dan alamatnya dipakai objek lain, nilai asli creature lama bisa
  terpakai.
- Instant Catch selalu menampilkan "Reeled in instantly!" walaupun belum ada ikan yang tersangkut, karena hasil
  `GetCatch` tidak dicek.
- Saran: buang `Mistlands_DvergrTownEntrance1` dari kandidat The Queen dan berhenti di kandidat pertama yang
  berhasil. Untuk interval breeding, panggil `CancelInvoke("Procreate")` lalu `InvokeRepeating` dengan interval
  baru, atau hapus bagian itu dari deskripsi fitur. Cek `GetCatch` sebelum menampilkan pesan.

### 43. World Time Scale dan Pause Time hilang setiap kali menu ESC ditutup

- Lokasi: [world_time.cpp:49-74](../src/unity/world_time.cpp#L49-L74),
  [world_time.cpp:121-124](../src/unity/world_time.cpp#L121-L124), dan
  [player_menu.cpp:1186-1192](../src/menu/submenu/player_menu.cpp#L1186-L1192).
- Penyebab: mod mengatur kecepatan lewat `Game.FadeTimeScale`, yang hanya mengisi `Game.m_timeScale`. Di build
  sekarang, `Menu.Hide` memanggil `Game.Unpause`, dan `Unpause` mengisi ulang `m_timeScale = 1`.
  `Game.UpdatePause` juga memaksa `Time.timeScale = 1` setiap frame selama ada pemain lain yang tersambung.
  Status pause di menu dibaca dari variabel mod sendiri (`s_time_scale`), bukan dari game.
- Akibat: setelah menu ESC dibuka lalu ditutup, waktu kembali berjalan normal, tetapi menu masih menampilkan
  "Resume Time". Menekannya hanya mengatur kecepatan ke nilai slider (default 1), jadi perlu dua kali tekan
  untuk pause lagi. Hal yang sama terjadi saat pemain lain bergabung.
- Saran: baca status untuk label dan toggle dari `Game.m_timeScale`. Kalau kecepatan harus bertahan, pasang
  ulang nilainya setelah `Game.Unpause`, misalnya lewat pengecekan di `Game.Update`.

### 44. Sepuluh nama prefab tidak ada di game

- Lokasi: [player_menu.cpp:1657-1664](../src/menu/submenu/player_menu.cpp#L1657-L1664),
  [player_menu.cpp:1736-1740](../src/menu/submenu/player_menu.cpp#L1736-L1740),
  [item_spawner.cpp:153-294](../src/unity/item_spawner.cpp#L153-L294), dan
  [exploration.cpp:28](../src/unity/exploration.cpp#L28).
- Cara cek: 200 nama prefab dan lokasi yang ditulis langsung di kode dibandingkan dengan daftar asset game di
  `valheim_Data/StreamingAssets/SoftRef/manifest` dan `manifest_extended` (6.507 nama prefab). Yang tidak ada:

  | Di mod | Nama di game | Dipakai di |
  | --- | --- | --- |
  | `HelmetAshlandsMedium` | `HelmetAshlandsMediumHood` | Quick Armor "Full Flametal Armor Set", katalog default |
  | `ArmorAshlandsMediumLegs` | `ArmorAshlandsMediumlegs` | Quick Armor "Full Flametal Armor Set", katalog default |
  | `MagicallyStuffedMushroom` | `MagicallyStuffedShroom` | Quick Foods "Spawn Stuffed Mushroom", katalog default |
  | `SwordNidhogg` | `SwordNiedhogg` | katalog default |
  | `AxeBerserkir` | `AxeBerzerkr` | katalog default |
  | `StaffRedDumortierite` | `StaffClusterbomb` (Staff of Fracturing) | katalog default |
  | `ArmorPaddedChest` | `ArmorPaddedCuirass` | katalog default |
  | `SizzlingBerryTart` | `SizzlingBerryBroth` | katalog default |
  | `Ashwood` | `Blackwood` | katalog default |
  | `Hildir_tower` | `Hildir_plainsfortress` | pin "Sealed Tower" dan Reveal All Traders |

- Akibat:
  - "Full Flametal Armor Set" hanya menambahkan chest dan cape. Helm dan celana gagal dengan pesan "Inventory is
    full or the item could not be added." ([item_spawner.cpp:804](../src/unity/item_spawner.cpp#L804)), jadi
    terlihat seperti inventory penuh. Isinya juga set Ask (`AshlandsMedium`), bukan Flametal (`HelmetFlametal`,
    `ArmorFlametalChest`, `ArmorFlametalLegs`).
  - "Spawn Stuffed Mushroom (10x)" tidak pernah menambahkan item.
  - Katalog default dipakai sampai scan ObjectDB selesai. Selama itu, sembilan entri di atas gagal saat dipilih.
    Label "Staff of the Wild" juga menunjuk ke `StaffLightning` (Dundr), padahal Staff of the Wild adalah
    `StaffGreenRoots`.
  - Pin "Sealed Tower" tidak pernah terpasang, tetapi notifikasi tetap "Pin requested", karena
    `discover_location` selalu mengembalikan true dan pesan di menu tidak mengecek hasilnya.
- Saran: ganti nama sesuai tabel. Untuk Item Spawner, tampilkan "prefab tidak ditemukan" saat
  `ObjectDB.GetItemPrefab` mengembalikan null, supaya tidak tertukar dengan inventory penuh.

### 45. Task async yang kena exception SEH dijalankan ulang setiap frame

- Lokasi: [task_mgr.cpp:69-76](../src/task_mgr.cpp#L69-L76),
  [exception_handler.cpp:39-49](../src/logger/exception_handler.cpp#L39-L49), dan
  [stack_trace.cpp:195-215](../src/logger/stack_trace.cpp#L195-L215).
- Penyebab: `tick_impl` menjalankan setiap task di dalam `TRY_CLAUSE`/`EXCEPT_CLAUSE`. Kalau task kena access
  violation, `__except` menangkapnya, tetapi coroutine tidak pernah sampai ke `final_suspend`, jadi `done()`
  tetap false dan task tidak dibuang. Project ini dikompilasi dengan clang-cl tanpa `/EHa`, jadi `catch (...)`
  bawaan coroutine tidak menangkap SEH. Di ABI coroutine Clang, indeks resume hanya diperbarui di titik suspend,
  jadi di frame berikutnya task dilanjutkan dari titik suspend terakhir. Untuk tugas `async_run` biasa, artinya
  seluruh fungsi diulang dari awal.
- Akibat: satu fungsi yang crash di tengah jalan diulang setiap frame sampai unload, termasuk efek samping
  sebelum titik crash (spawn, RPC, notifikasi). Setiap pengulangan juga menjalankan `new_stack_trace`
  (`StackWalk64`, lalu `SymFromAddr` dan `SymGetLineFromAddr64` untuk sampai 32 frame) sebelum pengecekan log
  ganda, jadi frame rate ikut turun. `m_frame_pointers` juga tidak dikosongkan sebelum stack baru dibaca. Kalau
  stack baru lebih pendek, sisa frame dari exception sebelumnya ikut tercetak dan ikut masuk hash dedup.
- Belum dipastikan: perilaku ini disimpulkan dari ABI coroutine Clang dan setting build, belum direproduksi di
  game. Pemicunya juga butuh access violation di kode mod, yang jarang karena kebanyakan fungsi mengecek null.
- Saran: di `EXCEPT_CLAUSE` milik `tick_impl`, tandai task yang gagal dan buang dari daftar tanpa me-resume lagi.
  Cek hash alamat crash sebelum membuat stack trace lengkap, dan kosongkan `m_frame_pointers` di awal
  `grab_stacktrace`.

### 46. Masalah kecil di World Modifiers, Tame All, scanner pattern JS, settings, dan kode mati

- World Modifiers ([player_menu.cpp:1254-1258](../src/menu/submenu/player_menu.cpp#L1254-L1258)): daftar nilai
  setiap modifier tidak punya `Default`, padahal enum `WorldModifierOption` di game punya `Default = 0`. Setelah
  misalnya Combat diubah ke Hard, Combat tidak bisa dikembalikan ke normal sendirian. Satu-satunya jalan adalah
  preset Normal, yang ikut mereset modifier lain.
- Tame All Deer, Boar, dan Wolf ([tamed.cpp:26-30](../src/features/tamed.cpp#L26-L30)) mencocokkan hasil
  `Character.GetHoverName`, yang sudah diterjemahkan lewat `Localization.Localize`. Jadi ketiga tombol ini hanya
  jalan kalau bahasa game Inggris. Hewan yang sudah dinamai juga tidak cocok, tetapi itu tidak masalah karena
  sudah jinak.
- `range::fullscan` ([range.cpp:176](../src/memory/range.cpp#L176)) membuat `range mem(r.base, regions.size())`,
  jadi setiap region dipindai sepanjang jumlah region, bukan `r.size`. Pola di luar awal region tidak pernah
  ketemu. Kalau jumlah region lebih besar dari ukuran region (region 4 KB dan proses dengan ribuan region), scan
  membaca melewati akhir region dan bisa kena access violation di thread pool, yang tidak punya `__try`. Hanya
  terpakai lewat `main_batch.fullscan_run` dari skrip JS.
- Pattern cache ([pattern_cache.cpp:14-23](../src/memory/pattern_cache.cpp#L14-L23) dan
  [pattern_batch.cpp:21-31](../src/memory/pattern_batch.cpp#L21-L31)): loop baca memakai
  `while (!stream.eof())`, jadi satu entri sampah ikut dimasukkan. Offset dari cache juga langsung dipakai tanpa
  mencocokkan ulang byte pattern, jadi setelah update game dengan ukuran modul yang sama, skrip JS bisa menerima
  alamat yang salah. Pattern bawaan mod hanya `FF 23` (gadget return address), jadi tidak terdampak.
- Settings ([menu_settings.cpp:29-58](../src/menu_settings.cpp#L29-L58) dan
  [menu_settings.cpp:94-120](../src/menu_settings.cpp#L94-L120)): `save()` menulis langsung ke
  `menu_settings.json` dengan mode `trunc`. Kalau game crash saat menulis, file tertinggal setengah. Saat load
  berikutnya parse gagal, semua setting diganti default, lalu file lama ditimpa tanpa cadangan.
- Kode mati: `call_hook.cpp` membuat alokasi RWX 1 KB di alamat tetap (`base + 0x20000000`) saat DLL dimuat,
  padahal `call_hook` tidak dipakai di mana pun. Jalur OpenGL di
  [graphic_manager.cpp:797-805](../src/graphic/graphic_manager.cpp#L797-L805) menulis `m_swapchain_methods[i]`
  setelah `reserve` saja (tanpa `resize`), yang termasuk undefined behavior. Jalur ini tidak pernah dipanggil,
  karena renderer hanya meminta DX11.
- Saran: tambahkan `Default` ke setiap daftar modifier. Untuk Tame All, cocokkan `Character.m_name` (misalnya
  `$enemy_deer`) atau nama prefab. Pakai `r.size` di `fullscan`, dan cek ulang byte pattern sebelum memakai
  offset dari cache. Tulis settings ke file sementara lalu rename. Hapus `call_hook` dan jalur OpenGL kalau
  memang tidak dipakai.

### 47. Login dan WebSocket server mematikan verifikasi sertifikat TLS

- Lokasi: [auth_client.hpp:344-349](../src/server/auth_client.hpp#L344-L349) dan
  [socket_client.hpp:52](../src/server/socket_client.hpp#L52).
- Penyebab: `http_post` di `auth_client` memasang `SECURITY_FLAG_IGNORE_UNKNOWN_CA`,
  `SECURITY_FLAG_IGNORE_CERT_DATE_INVALID`, dan `SECURITY_FLAG_IGNORE_CERT_CN_INVALID` untuk semua request HTTPS,
  termasuk server produksi. Komentarnya menyebut "dev/custom environments", tetapi tidak ada pengecekan
  `env_type`. Client WebSocket dibuat dengan `verify_certificate = false`. Sebaliknya, `dll_session` memakai
  verifikasi TLS normal.
- Akibat: siapa pun yang bisa mencegat koneksi (Wi-Fi publik, DNS palsu, proxy) bisa menyamar sebagai server
  tanpa ketahuan. Yang terkirim lewat jalur ini adalah `refresh_token` dari `session.dat` (header Cookie), token
  sesi (di query string URL WebSocket, [auth_client.hpp:183](../src/server/auth_client.hpp#L183)), dan detail
  hardware mentah (UUID board, serial board dan disk, MAC, MachineGuid). HWID juga ditulis ke file log
  ([auth_client.hpp:39](../src/server/auth_client.hpp#L39)).
- Cakupan: kode server hanya dikompilasi kalau `PRODUCTION` didefinisikan. CMake di repo ini tidak
  mendefinisikannya, jadi build biasa tidak terdampak. Build rilis yang memakai `-DPRODUCTION` terdampak.
- Saran: hapus flag `IGNORE_*` untuk produksi (kalau perlu, sisakan hanya untuk `env_type` lokal), pakai
  `verify_certificate = true` untuk WSS, kirim token lewat header, bukan query string, dan jangan menulis HWID ke
  log.

### 48. Hook dari skrip JS dihapus dua kali saat unload

- Lokasi: [trampoline_binding.cpp:39-57](../src/services/javascript/bindings/trampoline_binding.cpp#L39-L57),
  [trampoline_binding.cpp:298](../src/services/javascript/bindings/trampoline_binding.cpp#L298),
  [trampoline_binding.cpp:327-367](../src/services/javascript/bindings/trampoline_binding.cpp#L327-L367),
  [hooking.cpp:253-270](../src/hooking.cpp#L253-L270), dan
  [javascript_manager.cpp:14-21](../src/services/javascript/javascript_manager.cpp#L14-L21).
- Penyebab: `detour.add` dari skrip membuat `big::detour_hook`, yang otomatis masuk daftar global
  `detour_base::hooks()`, tetapi pemiliknya `unique_ptr` di map `g_hooks`. Saat unload,
  `hooking::disable()` mematikan semua hook di daftar itu lalu `delete_disabled_hooks()` menghapusnya dengan
  `delete`, termasuk hook JS. Setelah `FreeLibraryAndExitThread`, destructor `static` milik
  `javascript_manager` memanggil `trampoline::shutdown()`, lalu `g_hooks.clear()` memanggil
  `disable_immediately()` pada objek yang sudah dihapus dan menghapusnya lagi.
- Akibat: kalau ada skrip yang pernah memakai `detour.add`, unload berakhir dengan use-after-free dan double free
  di dalam `DllMain`, yang biasanya membuat game crash. Masalah lain di jalur yang sama:
  - Nama hook disimpan sebagai `std::string_view` ([detour_base.hpp:9](../src/hooking/detour_base.hpp#L9)), padahal
    untuk hook JS nama itu string sementara. Log di `hooking::disable` membaca memori yang sudah dibebaskan.
  - `JsDetourDispatch` memanggil callback JS dari thread mana pun yang memanggil fungsi yang di-hook. Pada saat
    yang sama skrip juga berjalan di thread game (eval) dan thread render (callback menu, bug 23). QuickJS tidak
    thread-safe, jadi pemakaian bersamaan bisa merusak heap JS.
  - `detour.disable(name)` dari dalam callback hook itu sendiri langsung membebaskan trampoline dan objek
    callback yang sedang berjalan.
- Saran: hapus hook JS di `g_hooks` sebelum `g_hooking->disable()` (misalnya panggil
  `javascript_manager::unload_all_modules` dan `trampoline::shutdown` di urutan unload `main.cpp`). Simpan nama
  hook sebagai `std::string`. Antre callback JS ke satu thread, atau kunci runtime QuickJS dengan mutex.

### 49. Bug di binding JavaScript

- Lokasi: [mono_binding.cpp:33-121](../src/services/javascript/bindings/mono_binding.cpp#L33-L121),
  [gui_binding.cpp:113-125](../src/services/javascript/bindings/gui_binding.cpp#L113-L125),
  [gui_binding.cpp:164-179](../src/services/javascript/bindings/gui_binding.cpp#L164-L179), dan
  [javascript_manager.cpp:203](../src/services/javascript/javascript_manager.cpp#L203).
- Masalah:
  - `mono.set_field_string` menulis pointer `const char*` milik C++ langsung ke field string managed, bukan
    `MonoString`. Kode game yang membaca field itu akan membaca memori yang salah dan bisa crash.
  - `mono.get_field_value` membaca ke variabel `void*` (8 byte). Untuk field yang lebih besar, misalnya
    `Vector3` (12 byte), `mono_field_get_value` menulis melewati variabel itu di stack.
  - `mono.invoke_method` mengirim angka desimal sebagai `double` 8 byte. Untuk parameter `float` (`Single`), Mono
    hanya membaca 4 byte pertama, jadi nilainya salah.
  - Slider integer dari `canvas` menulis nilainya ke `intStates`, tetapi callback-nya membaca
    `floatStates`, jadi skrip selalu menerima nilai awal. `precision` slider float tidak pernah diisi, jadi selalu
    0 desimal.
  - `canvas.add_submenu` tidak menaikkan `build_version`, padahal `register_gui` hanya mendaftarkan menu yang
    versinya berubah. Submenu dari skrip tidak pernah muncul.
  - REPL menjalankan kode sebagai module (QuickJS 2020-07-05), jadi hasil ekspresi selalu `undefined` dan tidak
    pernah tampil, dan `const` dari satu eval tidak terlihat di eval berikutnya.
- Saran: pakai `mono::to_mono_string` di `set_field_string`, baca field ke buffer seukuran
  `mono_field_get_type`, terima tipe argumen dari skrip, simpan nilai slider integer ke tempat yang dibaca
  callback, naikkan `build_version` di `add_submenu`, dan eval REPL sebagai script global.

### 50. Tombol Exit di Settings menutup game tanpa menyimpan

- Lokasi: [setting_menu.cpp:13-15](../src/menu/submenu/setting_menu.cpp#L13-L15).
- Penyebab: tombol Exit memanggil `exit(0)` langsung dari callback menu di thread render.
- Akibat: proses berhenti tanpa lewat jalur keluar Valheim, jadi world dan karakter tidak disimpan. Progres sejak
  autosave terakhir hilang, dan pemain lain di sesi yang di-host langsung terputus. `exit` juga menjalankan
  destructor `static` mod saat thread game masih berjalan, termasuk destructor `equipment_manager` yang memanggil
  Mono (bug 37).
- Saran: panggil jalur keluar game di thread game, misalnya `Menu.OnQuitYes` atau `Application.Quit` lewat
  `queue_job`, atau hapus tombol ini.

### 51. Masalah kecil di server, input, teleport, dan renderer Vulkan

- Server (hanya build `PRODUCTION`):
  - `dll_session_client` tidur 20 detik sekaligus di heartbeat
    ([dll_session.hpp:213-217](../src/server/dll_session.hpp#L213-L217)), dan destructor-nya menunggu thread itu.
    Unload bisa tertahan sampai 20 detik, ditambah request `/dll-session/close` dengan timeout 5 detik per tahap.
  - `socket_client::connect` mengisi `m_shutdown_requested = false`, lalu langsung memanggil `disconnect()`, yang
    mengisinya `true` lagi dan tidak pernah dikembalikan
    ([socket_client.hpp:35-46](../src/server/socket_client.hpp#L35-L46)). Akibatnya log koneksi tertutup dan
    error thread I/O tidak pernah ditulis.
  - `FORCE_LOGOUT` dideteksi dengan mencari substring di pesan apa pun, bukan field event.
  - Selama belum terautentikasi, heartbeat menjalankan `authenticate()` setiap 15 detik: dua request HTTP dan tiga
    kali pengumpulan HWID lengkap per percobaan.
- Input ([input_service.cpp:48-81](../src/services/input/input_service.cpp#L48-L81),
  [input.cpp:30-35](../src/menu/core/input.cpp#L30-L35)): stack callback dan buffer `m_input` dipakai dari thread
  render (menu) dan thread game (`execute` lewat `async_run`) tanpa kunci. Kotak input juga hanya bisa ditutup
  dengan Enter.
- Teleport: daftar Objectives memasukkan pin `Memorial` (type 17) sebagai "Hildir Quest", dan pin buatan pemain
  yang namanya mengandung "bog", "elder", atau "queen" ikut masuk
  ([teleport_submenu.cpp:249-280](../src/menu/submenu/teleport_submenu.cpp#L249-L280)). "Add Teleport" di Custom
  Teleport menyimpan ke variabel global `category`, bukan kategori submenu yang sedang dibuka
  ([teleport_submenu.cpp:364-387](../src/menu/submenu/teleport_submenu.cpp#L364-L387)).
- Vulkan (belum dipastikan): kalau mod masuk setelah swapchain dibuat, image view memakai
  `VK_FORMAT_R8G8B8A8_UNORM` ([render_vulkan.cpp:227-262](../src/render/render_vulkan.cpp#L227-L262)), padahal
  format swapchain bisa BGRA8 atau sRGB. Vulkan tidak mengizinkan format view berbeda untuk image swapchain tanpa
  flag mutable format. Kemungkinan akibatnya warna menu tertukar merah dan biru sampai swapchain dibuat ulang,
  atau error di driver yang ketat.
- Kode mati tambahan: kelas `plugins` (loader BepInEx), `spoof_call` beserta scan `FF 23` saat startup,
  `Rotator` (dengan `operator+` dan `operator/` yang salah), `command_arguments`, `checkbox_texture`,
  `alpha_service::send_hardware`, `list_get` di `mono_binding`, `home()`, `stats_submenu()`, dan
  `main_script.hpp` tidak dipakai.

## Performa

Kecuali disebut lain (P15 dan P18 berjalan di callback gambar), kode di bawah berjalan di thread game. Biaya
tambahan per frame langsung mengurangi FPS, dan alokasi objek managed menambah kerja GC Mono, yang
menghentikan game sejenak saat berjalan (stutter).

| # | Masalah | Kapan terasa | Lokasi |
| --- | --- | --- | --- |
| P1 | `get_local_player()` tanpa cache, dipanggil di hook yang sangat sering jalan (diperbaiki) | Selalu, makin berat dengan banyak musuh | `utility/unity.hpp`, banyak hook |
| P2 | Hook stat item membuat 2 objek managed per pemanggilan (diperbaiki) | Selalu (hotbar), lebih berat saat inventory terbuka | `hooks/item_stat_overrides.cpp` |
| P3 | ESP membaca semua karakter tanpa batas jarak dan semua drop tiap frame (diperbaiki) | Saat ESP menyala | `worker/entity_worker.cpp` |
| P4 | Marker bangunan rusak memakai 4 panggilan Mono per marker per frame (diperbaiki) | Saat Mark Damaged Buildings menyala | `unity/base_tools.cpp` |
| P5 | Data creature dikumpulkan tiap frame walaupun HUD inspector mati (diperbaiki) | Saat crosshair mengarah ke creature | `unity/animal_tools.cpp` |
| P6 | Noclip membaca dan menulis collider tiap frame walaupun mati (diperbaiki) | Selalu | `hooks/player_update.cpp` |
| P7 | `Hud.get_instance` dicari berdasarkan nama tiap frame (diperbaiki) | Selalu | `unity/hud.cpp` |
| P8 | Ukuran layar dibaca lewat 2 panggilan Mono tiap frame (diperbaiki) | Selalu | `worker/main_worker.cpp` |
| P9 | Inventory Size mengubah ukuran UI dan membuat 2 string setiap frame (diperbaiki) | Saat Inventory Size menyala | `features/inventory.cpp` |
| P10 | Setiap item yang diambil dari slot equipment menyimpan ulang semua halaman (diperbaiki) | Setiap tembakan kalau ammo di slot equipment | `hooks/equipment_hooks.cpp` |
| P11 | Override cuaca membuat string managed baru setiap frame (diperbaiki) | Saat cuaca dipaksa | `hooks/base_tools.cpp` |
| P12 | Forsaken Power Always Ready mencari class dan field setiap frame (diperbaiki) | Saat fitur menyala | `hooks/update_guardian_power.cpp` |
| P13 | Hash nama efek dihitung lewat Mono setiap kali dipakai (diperbaiki) | Saat Auto-Purge atau Prevent Freezing/Cold/Wet menyala | `unity/buff_tools.cpp` |
| P14 | Scan objek sekitar memakai `FindObjectsOfType` dan sort dengan panggilan Mono, setiap detik (diperbaiki sebagian) | Saat monitor Base Tools terbuka atau marker bangunan menyala | `unity/base_tools.cpp` |
| P15 | Jendela ImGui membaca data game lewat Mono setiap frame (diperbaiki) | Saat Status Effect Browser, Equipment, Item Spawner, Global Key Editor, atau World Bookmarks terbuka | `unity/status_effect_browser.cpp`, `unity/equipment_slots.cpp`, `unity/item_spawner.cpp`, `unity/global_key_editor.cpp`, `unity/bookmark_manager.cpp` |
| P16 | UI equipment native bekerja setiap frame, termasuk saat inventory tertutup | Selalu (Equipment Slots default menyala) | `unity/equipment_slots.cpp` |
| P17 | `save_to_player()` menulis log setiap kali, dan dipanggil per item saat Auto-Fill atau Return All | Setiap simpan | `unity/equipment_slots.cpp` |
| P18 | Information Overlay membaca 8 nilai lewat Mono setiap frame | Saat overlay menyala (menyala di setting Anda) | `menu/submenu/player_menu.cpp` |
| P19 | Spawn in World membuat satu objek jaringan per unit untuk item yang tidak bisa ditumpuk, sampai 10.000 dalam satu frame | Saat spawn item non-stack dalam jumlah besar | `unity/item_spawner.cpp` |
| P20 | `set_health` mencari method berdasarkan nama setiap kali dipanggil | Saat Maintain Health atau Health Regen menyala | `unity/character.cpp`, `hooks/player_update.cpp` |
| P21 | Pratinjau skrip di Javascript Console membaca file dari disk setiap frame | Saat tab Script Manager terbuka dan ada skrip yang dipilih | `menu/core/js_scripts.cpp` |

### P1. `get_local_player()` tanpa cache

- Lokasi: [unity.hpp:9-40](../src/utility/unity.hpp#L9-L40).
- Biaya per panggilan: cek attach thread, lock mutex cache image, membuat `std::string` dari
  `"assembly_valheim"` (16 karakter, lebih panjang dari buffer SSO, jadi alokasi heap), `mono_class_from_name`,
  lalu `mono_class_get_field_from_name` yang membandingkan nama field satu per satu di class `Player`.
- Pemanggil yang paling sering, dan semuanya menghitung `get_local_player()` sebelum mengecek setting yang
  murah:
  - [ghost_mode.cpp:8](../src/hooks/ghost_mode.cpp#L8): `InGhostMode` dipanggil AI musuh
    (`BaseAI.FindEnemy`, `CanSeeTarget`, `CanHearTarget`) setiap kali mengecek target pemain.
  - [parry_hooks.cpp:176](../src/hooks/parry_hooks.cpp#L176): `IsBlocking` dipanggil setiap frame oleh setiap
    humanoid (`Humanoid.UpdateBlock`, `CheckRun`).
  - [is_debug_flying.cpp:9](../src/hooks/is_debug_flying.cpp#L9): dipanggil `Character.UpdateMotion` setiap frame.
  - [god_mode.cpp:8](../src/hooks/god_mode.cpp#L8), [bow_draw.cpp:8](../src/hooks/bow_draw.cpp#L8),
    [bow_draw.cpp:18](../src/hooks/bow_draw.cpp#L18), [speed_factor.cpp:17](../src/hooks/speed_factor.cpp#L17),
    [speed_factor.cpp:34](../src/hooks/speed_factor.cpp#L34).
- Saran: simpan class, field, offset, dan alamat data statis dalam variabel `static`, sehingga
  `get_local_player()` cukup satu pembacaan memori. Di hook, cek setting dulu, baru bandingkan dengan pemain
  lokal.
- **Status**: diperbaiki. Alamat slot `Player.m_localPlayer` dihitung sekali lalu disimpan di
  `std::atomic` (diulang kalau class belum tersedia), sehingga setiap panggilan berikutnya hanya satu pembacaan
  memori. Hook ghost mode, god mode, debug flying, bow draw, dan `IsBlocking` sekarang mengecek setting lebih
  dulu. Kedua hook speed factor melewati pengecekan pemain lokal saat total pengalinya 1.

### P2. Hook stat item membuat 2 objek managed per pemanggilan

- Lokasi: [item_stat_overrides.cpp:6-32](../src/hooks/item_stat_overrides.cpp#L6-L32), memanggil
  [item_data.cpp:143-180](../src/unity/item_data.cpp#L143-L180).
- Biaya per panggilan: `to_mono_string(name)` membuat string managed, `mono_runtime_invoke` pada
  `TryGetValue` mem-box hasil `bool` menjadi objek baru, lalu string hasilnya dikonversi ke UTF-8.
- Frekuensi: `GetMaxDurability` dipanggil setiap frame untuk ikon hotbar (`HotkeyBar.UpdateIcons`) dan untuk
  setiap item di grid saat inventory terbuka (`InventoryGrid.UpdateGui`). `GetArmor`, `GetBaseBlockPower`,
  dan `GetDeflectionForce` dipanggil saat menghitung armor, block, dan parry. Hook
  [get_weight.cpp:7](../src/hooks/get_weight.cpp#L7) memakai fungsi yang sama, dan `GetWeight` dipanggil
  `Player.AutoPickup` untuk setiap drop di dekat pemain setiap frame, serta `Inventory.UpdateTotalWeight`
  untuk setiap item setiap kali inventory berubah.
- Saran: baca dulu jumlah isi `m_customData` (field `_count` pada Dictionary) dan langsung kembalikan nilai
  asli kalau kosong, karena hampir semua item tidak punya override. Simpan string key managed sekali saja.
- **Status**: diperbaiki. `get_stat_override` membaca `_count - _freeCount` dari Dictionary (nama field dicek
  di mscorlib game) dan langsung kembali kalau hasilnya 0. Key string tidak di-cache: game sendiri tidak pernah
  mengisi `m_customData`, hanya menyalin dan menyimpannya (`Inventory.AddItem`, `Inventory.OldSave`,
  `ZDOMan.ConvertInventories`), jadi jalur lambat hanya jalan untuk item yang memang punya override.

### P3. ESP membaca semua karakter dan semua drop tiap frame

- Lokasi: [entity_worker.cpp:32-150](../src/worker/entity_worker.cpp#L32-L150).
- Biaya: untuk setiap karakter yang dimuat game, tanpa batas jarak, ESP membaca posisi, titik atas, HP, dan
  nama (konversi string dari Mono) setiap frame. Untuk drop, posisi setiap drop dibaca dulu sebelum disaring
  60 m, lalu `Localization.Localize` dipanggil per drop per frame. Sebelum itu,
  `esp_projection::begin_frame` mencari `GameCamera.get_instance`, field `m_camera`,
  `Camera.WorldToViewportPoint` (lewat `get_method_exact`, yang membandingkan nama tipe parameter), dan
  `Camera.get_rect` berdasarkan nama setiap frame ([esp_projection.cpp:5-33](../src/unity/esp_projection.cpp#L5-L33)).
- Saran: tambahkan batas jarak untuk karakter, simpan nama dan level per objek lalu segarkan sekitar sekali
  per detik, cache hasil `Localize` berdasarkan nama prefab, dan simpan hasil lookup di `begin_frame` di
  variabel `static`.
- Catatan: `esp_projection::project` sendiri juga memanggil `Camera.WorldToViewportPoint` lewat Mono dan
  mem-box hasilnya untuk setiap titik (posisi dan titik atas setiap karakter dan drop).
- **Status**: diperbaiki.
  - [esp_projection.cpp](../src/unity/esp_projection.cpp) sekarang membaca `worldToCameraMatrix`,
    `projectionMatrix`, dan `rect` kamera sekali per frame (3 panggilan Mono), lalu memproyeksikan setiap titik
    dengan hitungan matriks di C++, memakai rumus yang sama dengan Unity: posisi clip dibagi `w`, dan `z`
    adalah jarak sepanjang arah depan kamera. Urutan field `Matrix4x4` (column-major) dicek di runtime game,
    dan rumusnya dicek dengan kamera uji di Python.
  - Lookup di `begin_frame` disimpan di variabel `static`.
  - Nama karakter dan label drop (termasuk hasil `Localize`) di-cache per objek, disegarkan sekali per detik,
    dan dibuang saat objek tidak terlihat lagi.
  - Batas jarak karakter tidak ditambahkan, karena akan mengubah apa yang ditampilkan ESP. Karakter di luar
    layar sudah dilewati sebelum data lain dibaca.
  - Sisa biaya per karakter: posisi (2 panggilan Mono), serta titik atas dan HP untuk yang terlihat di layar.

### P4. Marker bangunan rusak memakai `unity::world_to_screen`

- Lokasi: [base_tools.cpp:620-631](../src/unity/base_tools.cpp#L620-L631), memanggil
  [unity.hpp:882-906](../src/utility/unity.hpp#L882-L906).
- Biaya: setiap marker (sampai 200) memanggil `Camera.get_main`, `WorldToScreenPoint`, `get_screen_width`,
  dan `get_screen_height` lewat Mono, dan hasilnya di-box. Totalnya bisa 800 panggilan dan 600 alokasi per
  frame.
- Saran: pakai `esp_projection::project` yang dipakai ESP, setelah `esp_projection` diubah menjadi hitungan
  matriks di C++ (lihat P3).
- **Status**: diperbaiki. Marker memanggil `esp_projection::begin_frame` sekali, lalu `project` untuk setiap
  bangunan, jadi biayanya 3 panggilan Mono per frame berapa pun jumlah marker. Marker sekarang menyimpan
  koordinat viewport, dan panel mengalikannya dengan ukuran layar ImGui, sama seperti ESP.

### P5. Data creature dikumpulkan walaupun HUD inspector mati

- Lokasi: [animal_tools.cpp:66](../src/unity/animal_tools.cpp#L66).
- Biaya: setiap frame, selama crosshair mengarah ke creature, sekitar 12 nilai dibaca lewat Mono (nama,
  level, HP, posisi, tameness, love points, dan lainnya), tanpa mengecek apakah Show Animal Inspector HUD menyala.
- Saran: keluar lebih awal kalau HUD inspector dan hotkey tame sama-sama mati.
- **Status**: diperbaiki dengan cara sedikit berbeda. Snapshot yang sama dipakai tombol menu Tame, Heal, dan
  Breed Aimed Creature, jadi keluar lebih awal akan merusak tombol itu. Selama inspector mati, snapshot hanya
  menyimpan creature yang dibidik, dan nilai detail hanya dibaca saat inspector menyala. Nama untuk notifikasi
  Heal dan Breed dibaca saat aksi dijalankan.

### P6. Noclip membaca dan menulis collider tiap frame walaupun mati

- Lokasi: [player_update.cpp:83](../src/hooks/player_update.cpp#L83).
- Biaya: setiap frame, `GetCollider`, `get_enabled` (hasil di-box), dan `set_enabled` dipanggil, karena
  `active_collider` dikosongkan di akhir setiap frame saat noclip mati.
- Saran: kalau noclip mati dan tidak ada collider yang perlu dipulihkan, langsung keluar.
- **Status**: diperbaiki bersama #13. Saat noclip mati dan tidak ada collider yang ditahan, fungsi langsung
  keluar tanpa panggilan Mono.

### P7. `Hud.get_instance` dicari tiap frame

- Lokasi: [hud.cpp:361](../src/unity/hud.cpp#L361) (dan [hud.cpp:60](../src/unity/hud.cpp#L60)).
- Biaya: `mono::get_method` tanpa `static`, jadi daftar method `Hud` ditelusuri berdasarkan nama setiap frame.
- Saran: jadikan `static`, seperti pola di file lain.
- **Status**: diperbaiki. Lookup `Hud.get_instance`, `m_userHidden`, dan `SetVisible` sekarang `static`.

### P8. Ukuran layar dibaca tiap frame

- Lokasi: [main_worker.cpp:35-36](../src/worker/main_worker.cpp#L35-L36).
- Biaya: 2 panggilan Mono dan 2 alokasi box per frame. Kecil, tetapi tidak perlu.
- Saran: baca sekali per detik, misalnya di `slow_run`.
- **Status**: diperbaiki. Ukuran layar dibaca di `safe_slow_update`, sekali per detik. Setelah resolusi
  diganti, menu memakai ukuran lama paling lama 1 detik.

### P9. Inventory Size bekerja penuh setiap frame

- Lokasi: [inventory.cpp:88-109](../src/features/inventory.cpp#L88-L109).
- Biaya: setiap frame, semua item di inventory dibaca untuk mencari baris terbawah,
  `InventoryGui.SetInventorySize` dipanggil (mengubah `sizeDelta` RectTransform), class dan field
  `InventoryGui.m_instance` dicari berdasarkan nama tanpa cache, dan `AddUniqueKeyValue` dipanggil dengan 2
  string managed baru.
- Saran: terapkan ukuran hanya saat setting berubah atau saat inventory berganti, bukan setiap frame.
- **Status**: diperbaiki. Ukuran inventory, `invrows`, dan ukuran panel hanya diterapkan saat setting berubah,
  inventory berganti, atau ukuran inventory saat ini berbeda dari yang terakhir diterapkan. Kasus terakhir itu
  mencakup game yang mengubahnya lewat `Player.SetInventorySize` saat spawn atau saat membeli baris di trader.
  Panel juga diterapkan ulang saat instance `InventoryGui` berganti. Hanya `SetInventorySize` yang menulis
  ukuran panel itu, jadi cukup diterapkan sekali. Setiap frame sisa dua pembacaan field ukuran.

### P10. Item dari slot equipment memicu `save_to_player()` setiap kali diambil

- Lokasi: [equipment_hooks.cpp:111-140](../src/hooks/equipment_hooks.cpp#L111-L140), memanggil
  [equipment_slots.cpp:512](../src/unity/equipment_slots.cpp#L512).
- Biaya: setiap `Inventory.RemoveItem(ItemData, int)` yang mengambil dari inventory equipment langsung
  menyerialisasi semua halaman equipment (ZPackage, `Inventory.Save`, Base64, lalu `m_customData`). Di game,
  method ini dipakai `Attack.UseAmmo`, jadi kalau ammo ditaruh di slot equipment, ini terjadi di setiap
  tembakan. Hal yang sama berlaku saat memasukkan bahan bakar atau bijih satu per satu.
- Saran: tandai sebagai kotor dan simpan sekali per beberapa detik, atau saat game menyimpan (`player_save`).
- **Status**: diperbaiki. Hook `RemoveItem` sekarang hanya memanggil `mark_dirty()`. `equipment_manager::tick`
  menyimpan paling cepat 2 detik setelah perubahan pertama. `player_save` tetap menyimpan langsung, dan setiap
  penyimpanan yang berhasil menghapus tanda kotor. Kalau mod di-unload kurang dari 2 detik setelah item terakhir
  diambil dan game tidak sempat menyimpan, perubahan terakhir itu tidak ikut tersimpan.

### P11. Override cuaca membuat string managed baru setiap frame

- Lokasi: [base_tools.cpp:49-56](../src/hooks/base_tools.cpp#L49-L56).
- Biaya: `EnvMan.GetEnvironmentOverride` dipanggil setiap frame, dan selama cuaca dipaksa hook ini memanggil
  `to_mono_string(cfg.weather)` setiap kali. Selain itu, semua hook produksi dan cuaca memanggil
  `get_options()`, yang mengunci mutex dan menyalin seluruh struct opsi termasuk string.
- Saran: simpan string managed cuaca sekali (dengan GC handle) dan buat ulang hanya saat pilihannya berubah.
- **Status**: diperbaiki. String cuaca ditahan dengan GC handle dan baru dibuat ulang kalau nama cuacanya
  berubah. Hook produksi (smelter, beehive, plant, cooking, sap) dan `multiplier()` sekarang memakai
  `base_tools::option(&options::field)`, yang hanya menyalin satu field di bawah mutex, bukan seluruh struct
  beserta stringnya. `environment_update` tetap menyalin struct karena butuh beberapa field, tetapi hanya sekali
  per FixedUpdate.

### P12. Forsaken Power Always Ready mencari class dan field setiap frame

- Lokasi: [update_guardian_power.cpp:9-12](../src/hooks/update_guardian_power.cpp#L9-L12).
- Biaya: `mono::get_class` dan `mono::get_field` dipanggil tanpa `static` di setiap `UpdateGuardianPower`,
  dan `get_local_player()` dihitung sebelum setting dicek.
- Saran: jadikan lookup `static`, dan cek setting lebih dulu.
- **Status**: diperbaiki. Field `m_guardianPowerCooldown` disimpan di `static`, dan setting dicek sebelum
  pemain lokal.

### P13. Hash nama efek dihitung lewat Mono setiap kali dipakai

- Lokasi: [buff_tools.cpp:20-40](../src/unity/buff_tools.cpp#L20-L40), dipakai
  [buff_tools.cpp:574-592](../src/unity/buff_tools.cpp#L574-L592) dan
  [buff_tools.cpp:637-667](../src/unity/buff_tools.cpp#L637-L667).
- Biaya: setiap hash membuat string managed, memanggil `GetStableHashCode` lewat Mono, dan mem-box hasil `int`.
  Auto-Purge melakukan ini untuk 10 nama efek setiap frame, ditambah 10 panggilan `RemoveStatusEffect`,
  `get_local_player()`, dan `GetSEMan`. Prevent Freezing/Cold/Wet melakukan hal yang sama untuk 3 nama setiap
  frame.
- Saran: hash nama efek tidak berubah, jadi hitung sekali dan simpan. `SEMan` punya field statis seperti
  `s_statusEffectWet` yang sudah berisi hash-nya.
- **Status**: diperbaiki. `get_stable_hash` menyimpan hasil per nama dalam cache yang dijaga mutex, jadi
  setiap nama hanya dihitung sekali. Berkat hook #21, Auto-Purge dan Prevent Freezing/Cold/Wet juga tidak lagi
  menghapus efek setiap frame.

### P14. Scan objek sekitar memakai `FindObjectsOfType` setiap detik

- Lokasi: [base_tools.cpp:90-121](../src/unity/base_tools.cpp#L90-L121), dipanggil dari
  [base_tools.cpp:643](../src/unity/base_tools.cpp#L643) setiap detik.
- Biaya: setiap `nearby()` memanggil `Object.FindObjectsOfType`, yang memindai semua objek di scene, lalu
  membaca posisi setiap hasil lewat 2 panggilan Mono dan 1 alokasi box. Hasilnya diurutkan dengan comparator
  yang membaca posisi lagi lewat Mono di setiap perbandingan. Satu update bisa memanggil `nearby()` untuk
  sampai 9 tipe. Di base besar dengan banyak `WearNTear`, ini membuat game tersendat sekali per detik.
- Saran: pakai daftar instance milik game (misalnya `WearNTear.s_allInstances`), baca posisi sekali per objek
  sebelum mengurutkan, dan pisahkan scan per tipe sesuai monitor yang terbuka.
- **Status**: diperbaiki sebagian.
  - `WearNTear` dan `ShieldGenerator` sekarang dibaca dari daftar instance game (`s_allInstances` dan
    `m_instances`). Hanya dua class itu yang punya daftar statis; tipe lain tetap memakai `FindObjectsOfType`.
  - Posisi setiap objek dibaca sekali sebelum diurutkan.
  - Scan belum dipisah per monitor yang terbuka.

### P15. Jendela ImGui membaca data game lewat Mono setiap frame

- Lokasi: [status_effect_browser.cpp:74](../src/unity/status_effect_browser.cpp#L74) dan
  [equipment_slots.cpp:2026](../src/unity/equipment_slots.cpp#L2026).
- Biaya: Status Effect Browser memanggil `unity::get_name(effect)` (panggilan Mono dan konversi string) untuk
  setiap efek di daftar, setiap frame selama jendela terbuka. Jendela Equipment memanggil
  `Inventory.GetItemAt` lewat Mono untuk setiap sel grid setiap frame, ditambah `InventoryGui.IsVisible`.
  Item Spawner memanggil `initialize()` setiap frame, yang menjalankan `ObjectDB.get_instance` lewat
  `mono_runtime_invoke` ([item_spawner.cpp:328](../src/unity/item_spawner.cpp#L328),
  [item_spawner.cpp:1167](../src/unity/item_spawner.cpp#L1167)). Global Key Editor memanggil
  `ZoneSystem.GetGlobalKeys`, yang membuat list baru, lalu mengonversi setiap key ke string, setiap frame
  ([global_key_editor.cpp:38-50](../src/unity/global_key_editor.cpp#L38-L50)). World Bookmarks membaca
  `ZNet.m_world`, memanggil `Dictionary.TryGetValue` lewat Mono, lalu mem-parse JSON semua bookmark setiap frame
  ([bookmark_manager.cpp:159-193](../src/unity/bookmark_manager.cpp#L159-L193)). Semuanya berjalan di thread
  render (bug 20).
- Saran: ambil daftar nama dan isi slot sekali saat jendela dibuka atau saat inventory berubah, lalu gambar
  dari data yang sudah disimpan.
- **Status**: diperbaiki. Setiap jendela sekarang menggambar dari salinan data yang diisi di thread game:
  - Status Effect Browser: efek aktif disegarkan setiap 500 ms, efek terdaftar sekali setiap jendela dibuka.
  - Equipment: isi slot, status equipped, stack, durability, visibilitas inventory, dan daftar picker
    disegarkan setiap 250 ms dan langsung setelah setiap aksi. `init_or_sync()` tidak lagi dipanggil dari
    thread render, karena `tick()` sudah menjalankannya.
  - Global Key Editor: daftar key disegarkan sekali per detik.
  - World Bookmarks: daftar bookmark disegarkan saat jendela dibuka, sekali per 2 detik, dan setelah Save atau
    Delete.
  - Item Spawner: `ObjectDB.get_instance` tidak lagi dipanggil setiap frame setelah scan otomatis dimulai.

### P16. UI equipment native bekerja setiap frame

- Lokasi: [equipment_slots.cpp:1869-1963](../src/unity/equipment_slots.cpp#L1869-L1963) dan
  [equipment_slots.cpp:1637-1867](../src/unity/equipment_slots.cpp#L1637-L1867).
- Biaya:
  - Saat inventory tertutup (hampir selalu), setiap frame tetap ada `get_inventory_gui`, `IsVisible`,
    `get_gameObject`, dan `SetActive(false)` walaupun grid sudah tidak aktif.
  - Saat inventory terbuka, `decorate_native_slots` berjalan untuk setiap slot setiap frame: sekitar 9
    panggilan Mono (`get_Position`, `get_gameObject`, `SetActive`, `get_transform`, `Transform.Find("binding")`,
    `GetComponent`, `set_text`, `set_enabled`) dan 2-3 string baru per slot. `get_class` untuk
    `InventoryGrid`, `InventoryElement`, dan `TMP_Text` juga diulang setiap frame.
- Saran: simpan status aktif terakhir dan panggil `SetActive` hanya saat berubah. Hias slot sekali saat grid
  dibuat atau saat isi slot berubah, dan simpan komponen teks per slot.

### P17. `save_to_player()` menulis log setiap kali dan dipanggil per item

- Lokasi: [equipment_slots.cpp:653](../src/unity/equipment_slots.cpp#L653),
  [equipment_slots.cpp:858-972](../src/unity/equipment_slots.cpp#L858-L972).
- Biaya: setiap simpan menulis `LOG(INFO) "Successfully saved equipment pages..."`. Digabung dengan P10, itu
  berarti satu baris log per tembakan kalau ammo ada di slot equipment. Auto-Fill dan Return All memanggil
  `move_to_slot` atau `move_from_slot_to_inventory` per item, dan masing-masing menyimpan semua halaman.
- Saran: hapus log sukses, dan simpan sekali di akhir Auto-Fill atau Return All.

### P18. Information Overlay membaca 8 nilai lewat Mono setiap frame

- Lokasi: [player_menu.cpp:312-340](../src/menu/submenu/player_menu.cpp#L312-L340).
- Biaya: posisi, HP, max HP, max stamina, dan max eitr dibaca lewat panggilan Mono setiap frame, di callback
  gambar (thread render, lihat bug 20), ditambah 4 `std::format`. Default-nya mati, tetapi di setting Anda
  menyala.
- Saran: ambil nilai ini di thread game (misalnya di `main_worker`) dan biarkan overlay hanya menggambar.

### P19. Spawn in World tanpa batas jumlah objek

- Lokasi: [item_spawner.cpp:1011-1048](../src/unity/item_spawner.cpp#L1011-L1048).
- Biaya: jumlah dibatasi 10.000 unit, dan setiap stack menjadi satu `ItemDrop.DropItem`. Untuk senjata atau
  armor (stack 1), itu berarti 10.000 objek jaringan dibuat dalam satu frame, disusun 5 kolom ke arah sumbu
  dunia (+X dan +Z, bukan arah hadap pemain) sampai sekitar 1,6 km panjangnya. Game berhenti lama, dan semua
  ZDO baru dikirim ke server. Spawn objek non-item di fungsi yang sama sudah dibatasi 20.
- Saran: batasi jumlah drop per spawn, atau sebar pembuatannya ke beberapa frame dengan `co_await`.

### P20. `set_health` mencari method berdasarkan nama setiap kali dipanggil

- Lokasi: [character.cpp:23-61](../src/unity/character.cpp#L23-L61), dipanggil setiap frame dari
  [player_update.cpp:272-280](../src/hooks/player_update.cpp#L272-L280). Pola yang sama ada di
  `player::set_max_health`, `set_max_stamina(float, bool)`, `add_eitr`, dan `add_stamina`
  ([player.cpp:6-123](../src/unity/player.cpp#L6-L123)).
  Selama Free Crafting atau Open All Recipes menyala, `crafting_tools::refresh_ui` juga mencari 3 method lewat
  nama dan menyalakan tab Upgrade setiap frame ([crafting_tools.cpp:142-166](../src/unity/crafting_tools.cpp#L142-L166)).
- Biaya: `mono::get_method` tidak disimpan di variabel `static`, jadi setiap panggilan mencari class lewat
  `mono_class_from_name` lalu menelusuri semua method `Character` berdasarkan nama. Selama Maintain Health atau
  Health Regen menyala, ini terjadi setiap frame. Biayanya kecil per frame, tetapi perbaikannya cukup menambah
  `static`.
- Saran: simpan hasil `get_method` di variabel `static`, seperti fungsi lain di file yang sama.

### P21. Pratinjau skrip membaca file dari disk setiap frame

- Lokasi: [js_scripts.cpp:297-334](../src/menu/core/js_scripts.cpp#L297-L334).
- Biaya: selama tab Script Manager terbuka dan ada skrip yang dipilih, `read_script(path)` membuka dan membaca
  seluruh file, dan `std::filesystem::file_size` dipanggil, setiap frame di thread render. Untuk skrip kecil
  biayanya kecil, tetapi tetap I/O disk per frame.
- Saran: baca isi file sekali saat skrip dipilih atau saat tombol Reload ditekan.

## Sudah dicek dan aman

- Setting build dan pickup range: default mod sama dengan default game.
- Kamera: nilai asli direkam lalu dikembalikan, dan setting hanya diterapkan lewat tombol.
- Max Health, Max Stamina, Max Carry, stamina regen, dan eitr regen: menulis field base dan mengembalikan
  nilai yang direkam.
- Hook kecepatan smelter, cooking, beehive, dan sap collector: hanya mengalikan delta saat setting di atas 1x.
- Hook serangan, biaya stamina dan eitr, parry, durability, dan bow draw: semuanya mengecek pemain lokal.
- Tick equipment slots: hanya membaca hotkey.
- Polling `settings.json` oleh Astra: berjalan di thread terpisah sekali per detik, bukan di thread game.
- Gambar ESP di layar: memakai data yang sudah disiapkan dan proyeksi matriks di C++, bukan panggilan Mono.
- Semua target hook yang dicari berdasarkan nama dan jumlah parameter menunjuk ke overload yang benar
  (dicek untuk 20 hook yang punya overload, termasuk `Inventory.RemoveItem`, `EquipItem`, dan `DropItem`).
- Handler equip dan unequip di equipment slots: sudah mengecek pemain lokal di dalam `equipment_manager`.
- Pembelian di trader saat inventory penuh: koin hanya dipotong kalau item berhasil masuk.
- Hook damage, parry, dan block attack: hanya berjalan saat ada serangan, bukan setiap frame.
- `task_mgr` dan `async_run`: satu alokasi frame coroutine per tugas, dibersihkan setelah selesai.
- Fitur fishing: daftar pelampung diambil dari `FishingFloat.m_allInstances`, bukan `FindObjectsOfType`.
- Crafting speed: lookup `InventoryGui` sudah `static`, biaya per frame hanya satu panggilan dan dua tulisan
  field.
- `invoke_method` tidak pernah membiarkan exception managed lolos ke kode C++, jadi exception tidak membuat
  game crash.
- Pencarian Generated Locations: menelusuri ribuan lokasi, tetapi lewat `queue_job` di thread game dan hanya
  saat tombol ditekan.
- Lambda `fullscan_run` di binding sigscanner JS: menangkap referensi ke `pattern_batch` statis, jadi tidak
  menggantung.
- Sisa kode debug: satu-satunya `if (true)` di kode mod ada di `have_empty_slot.cpp` (bug 16, file sudah
  dihapus). `while (true)`
  di `zone_system.cpp` selalu berhenti saat enumerator habis.
- Layout `HitData.DamageTypes` yang ditulis langsung oleh hook damage dan parry: 11 float pertama cocok dengan
  build sekarang. Field ke-12, `m_nonPlayer`, tidak ikut ditulis, dan tidak relevan untuk jalur ini.
- Field `SharedData` yang diubah sementara oleh hook parry (block power, timed bonus, use durability) selalu
  dikembalikan setelah fungsi asli selesai.
- Pertukaran item di slot equipment memakai slot sementara (3, 0), yang bertipe `none` dan tersembunyi, jadi
  biasanya kosong.
- Aksi di Inventory Editor (arrange, duplicate, drop, delete, equip) berjalan lewat `async_run`. Tombol spawn
  di Item Spawner juga lewat `async_run`, tetapi jendelanya tetap membaca `ObjectDB` setiap frame (P15).
- Ukuran tipe untuk semua 156 akses field bertipe di seluruh kode dicek otomatis ke runtime Mono game. Semua
  cocok kecuali `m_worldLevel` (bug 33). Enum (`ItemType`, `Plant.Status`) berbasis `int` dan aman.
- Semua 499 lookup method dan field berdasarkan nama dicek otomatis ke build sekarang. Semua lookup utama
  ditemukan. Yang gagal hanya fallback untuk versi game lama, kecuali tiga di bug 34.
- Signature `Inventory.Changed(bool success, bool cheatedStateChanged)`, `Minimap.AddPin` (7 parameter), dan
  `Tameable.Command(Humanoid, bool)` cocok dengan argumen yang dikirim mod.
- Semua 32 pemanggilan `get_method_exact` dicek dengan meniru logika pencocokannya terhadap runtime game. Semua
  menemukan overload yang benar kecuali bug 41. Yang gagal lainnya hanya fallback untuk versi game lama.
- Overload yang dipilih lewat nama dan jumlah parameter di Inventory Editor benar: `RemoveItem(ItemData, int)`,
  `AddItem(ItemData)`, `AddItem(GameObject, int)`, dan `ItemDrop.DropItem(ItemData, int, Vector3, Quaternion)`.
- Signature `SEMan.AddStatusEffect`, `RemoveStatusEffect`, dan `GetStatusEffect` cocok dengan argumen mod,
  termasuk `variant` bertipe `Int16`.
- Struct yang dikirim ke `ZoneSystem.FindClosestLocation` cocok dengan layout `LocationInstance` (referensi,
  `Vector3`, `bool`). Buffer kosong 64 byte untuk `PlatformUserID` (16 byte) di `Minimap.AddPin` sama dengan
  nilai default.
- Ekspansi slot makanan di HUD mengganti ketiga array sekaligus tanpa jeda, jadi `UpdateFood` tidak pernah
  melihat panjang array yang berbeda.
- Layanan Mono: helper variadic `mono::invoke` hanya dipakai dengan argumen value type, jadi pointer yang
  dikirim benar. Cache image memakai `std::string` sebagai key. Helper `mono::list<T>` hanya dipakai untuk satu
  jenis `List` per `T`, jadi field `_items` dan `_size` yang di-cache tetap cocok.
- Enum `SkillType` di mod cocok dengan nilai di game, termasuk `Ride = 110` dan `All = 999`.
- Equipment slots: pemetaan slot ke `ItemType` cocok dengan enum, dan signature konstruktor halaman
  `Inventory(string, Sprite, int, int)`, `MoveItemToThis` (2 dan 5 parameter), serta
  `InventoryGrid.UpdateInventory(Inventory, Player, ItemData)` cocok dengan argumen yang dikirim mod.
- Creature dan Prefab Spawner memakai `ZDOMan.CreateNewZDO` lalu `ZNetScene.CreateObject`, sama dengan cara
  game membuat objek dari ZDO. ZDO-nya dimiliki client sendiri.
- Pickup Nearby Drops tidak menduplikasi item: drop milik client lain ditolak oleh `ItemDrop.CanPickup`.
- Optimize Old Terrain, Generate Missing Locations, Spawn Location, dan Upgrade WorldGen memanggil method yang
  sama dengan console command game. WorldGen v2 sama dengan versi sekarang, jadi tidak menurunkan versi world
  baru. Raid tertentu hanya bisa dimulai oleh host.
- Panel Online Players dan Base Tools hanya membaca snapshot yang disiapkan di thread game.
- Renderer DX11: render target dibuat sekali, resize ditangani lewat `pre_reset`/`post_reset` termasuk saat
  `ResizeBuffers` gagal, dan render target game disimpan lalu dikembalikan setiap frame.
- Renderer Vulkan: framebuffer, command buffer, fence, dan semaphore dibuat sekali. `QueueWaitIdle` hanya
  dipakai saat upload tekstur.
- Hook kecepatan lari dan jog, damage, incoming damage, serta biaya stamina dan eitr hanya mengalikan nilai dari
  game, jadi di 1x perilakunya sama dengan vanilla (beda dengan bug animasi yang sudah diperbaiki).
- Swim speed dan jump force menulis field base yang tidak diubah game, dikalikan dari nilai yang direkam.
- Ghost mode, god mode, flying, no encumbrance, EXP multiplier, freeze skill, dan Forsaken Power Always Ready
  mengecek pemain lokal.
- Override global key 21, 22, dan 23 cocok dengan `AllPiecesUnlocked`, `NoWorkbench`, dan `AllRecipesUnlocked`
  di enum `GlobalKeys`.
- Creature spawner dan item spawner hanya membaca prefab, tidak menulis field-nya.
- Base Tools: 34 method yang dipanggil lewat nama dan 23 field yang dibaca dengan tipe tertentu cocok dengan
  build sekarang, termasuk `Fireplace.m_maxFuel` yang `float` sementara `m_maxFuel` di Smelter, CookingStation,
  dan ShieldGenerator `int`. `Plant.Grow` mengembalikan `GameObject`, jadi hitungan "Instantly matured" benar.
- World Time dan Camera: signature `Game.FadeTimeScale`, `Game.CanPause`, `ZNet.SetNetTime(double)`,
  `GetTimeSeconds`, `SaveWorldAndPlayerProfiles`, `GameCamera.InFreeFly` (static), `ToggleFreeFly`,
  `SetFreeFlySmoothness`, dan `Player.TeleportTo` cocok. `EnvMan.m_dayLengthSec` dan `World.m_uid` (`Int64`)
  dibaca dengan ukuran yang benar, dan `ZNet.m_world` memang field static.
- `task_mgr`: coroutine dibuat dalam keadaan suspend (`initial_suspend` mengembalikan `suspend_always`) dan baru
  dijalankan dari `Game.Update`, jadi `async_run` dan `queue_job` benar-benar pindah ke thread game. Transfer
  ke parent di `final_suspend` juga benar.
- Selain bug 44, semua 200 nama prefab dan lokasi yang ditulis langsung di kode ada di daftar asset game,
  termasuk lokasi boss (`Eikthyrnir`, `GDKing`, `Bonemass`, `Dragonqueen`, `GoblinKing`,
  `Mistlands_DvergrBossEntrance1`, `FaderLocation`, `DN_Bossroom`), lokasi trader (`Vendor_BlackForest`,
  `Hildir_camp`, `BogWitch_Camp`, `Hildir_crypt`, `Hildir_cave`), dan prefab trader untuk Remote Merchant.
- Override cuaca dengan nama environment yang tidak ada tidak membuat crash, karena
  `EnvMan.QueueEnvironment(string)` mengecek hasil `GetEnv` sebelum dipakai.
- Hook DX11: jumlah entri vtable dummy (18 swapchain, 43 device, 144 context) sesuai interface, dan index
  Present (8) serta ResizeBuffers (13) benar.
- Command `int`, `float`, `list`, `vector3`, dan `color` menjalankan `on_change` lewat `async_run`, jadi
  callback selalu melihat nilai baru di thread game.
- Server: `dll_session` memakai verifikasi TLS normal, memvalidasi tiket (format, PID, waktu mulai proses), dan
  menghapus token dari memori saat selesai. Autentikasi berjalan di thread utama mod saat startup dan heartbeat di
  thread sendiri, jadi thread game tidak pernah menunggu jaringan.
- Antrean notifikasi Astra memakai mutex, jadi notifikasi dari thread jaringan dan thread game aman.
- Proxy `version.dll`: setiap fungsi yang diteruskan punya cache pointer sendiri, dan DLL asli dimuat dari folder
  System32.
- Data ESP memakai triple buffer yang dikunci selama thread render membaca, dan `esp_projection` menolak titik
  di belakang kamera, jadi label dan garis tidak tergambar terbalik.
- Renderer Vulkan memilih GPU sesuai `SystemInfo.graphicsDeviceVendorID` dan `graphicsDeviceID` dari Unity, jadi
  aman di PC dengan beberapa GPU.
- Semua field `self` di `menu_settings.hpp` ikut disimpan, lewat makro JSON atau `serialize_extra_options`.
- `file_manager` menolak path absolut dan `..`.
- Tekstur PNG yang ditanam di kode (`toggle_texture`, `checkbox_texture`) ukurannya cocok dan diakhiri chunk
  `IEND`.
- QuickJS versi 2020-07-05 menjalankan module secara sinkron, jadi error skrip tetap tertangkap `try`/`catch` dan
  masuk ke console.

## Cakupan audit

Sudah dibaca seluruhnya:

- Semua 46 file di `hooks/`, termasuk ukuran IL target (risiko inline) dan overload yang dipilih.
- Semua jalur per frame: `Game.Update` (task, `main_worker`, ESP worker), hook `Player.Update`, dan hook yang
  dipanggil game per frame atau per karakter.
- Semua fitur looped di `features/`, termasuk `on_disable` dan nilai default yang dipulihkan.
- Pekerjaan per detik (`slow_run`: Base Tools dan Online Players) dan polling settings Astra.
- Semua callback gambar yang didaftarkan di `gui.cpp`, kecuali panel Cheat Status.
- Seluruh `animal_tools`, `buff_tools`, `crafting_tools`, `hud`, `item_icons`, `inventory_editor`, `tameable`,
  `procreation`, `zone_system`, `status_effect_browser`, `exploration`, `ship_tools`, `online_players`,
  `localization`, `food`, `minimap`, `fishing_tools`, `monster_ai`, dan `inventory` di `unity/`.
- Seluruh `utility/unity.hpp`, layanan Mono (`mono.cpp`, `mono.hpp`, `array.hpp`), dan wrapper `character`,
  `player`, `item_data`, `item_drop`, serta `skills`.
- Seluruh `equipment_slots.cpp` (2.262 baris, fitur default menyala): sinkronisasi, simpan dan muat, equip dan
  unequip, utility tambahan, pemindahan item, tick, pembuatan dan dekorasi grid native, dan fungsi pembantu.
  Perilaku grid hasil salinan dicek ke IL `InventoryGrid` dan `InventoryGui.Awake` di build sekarang.
- Jalur unload di `main.cpp` dan `commands`, thread yang dipakai menu dan toggle, serta penanganan exception.
- Renderer DX11 seluruhnya, dan jalur per frame renderer Vulkan.
- Item, creature, dan prefab spawner, world tools, global key editor, serta custom teleport (service dan
  submenu) dibaca seluruhnya.
- Seluruh `menu/submenu/player_menu.cpp` (semua builder submenu dan opsinya), `base_tools_menu.cpp`, dan
  `menu/core/js_scripts.cpp`.
- Seluruh `unity/base_tools.cpp`, `world_time.cpp`, `camera_tools.cpp`, dan `bookmark_manager.cpp`.
- Infrastruktur: `task.hpp`, `task_mgr.cpp`, `thread_pool.cpp`, `logger/` (`logger.cpp`, `stack_trace.cpp`,
  `exception_handler.cpp`), semua tipe command di `commands/`, `menu_settings.cpp`, pattern scanner dan cache di
  `memory/`, helper hook di `hooking/`, dan `graphic/graphic_manager.cpp`.
- Putaran 100%: seluruh `server/` dan `utility/hwid.hpp`, seluruh engine dan binding JavaScript
  (`javascript_manager`, `mono_binding`, `trampoline_binding`, `command_binding`, `gui_binding`,
  `javascript_binding`, `store_binding`), renderer Vulkan seluruhnya (termasuk bootstrap, attach, dan shutdown),
  sisa `menu/` (`teleport_submenu.cpp`, `esp.cpp`, halaman kecil di `menu/core`, `setting_menu.cpp`),
  `file_manager`, proxy `version.dll`, `plugins.hpp`, `input_service`, tekstur UI, dan semua header (`unity/`,
  `commands/`, `memory/`, `hooking/`, `class/`, `menu_settings.hpp`, dan lainnya).
- Pemeriksaan otomatis untuk seluruh kode: ukuran tipe setiap akses field, keberadaan setiap method dan field
  yang dicari berdasarkan nama, dan nama prefab serta lokasi yang ditulis langsung. Dua yang pertama
  dibandingkan dengan runtime game, yang ketiga dengan daftar asset game.

Sengaja tidak diaudit:

- Panel dan tool Cheat Status (`unity/cheat_status.cpp`, `menu/submenu/cheat_status_menu.cpp`).

Di luar hitungan (bukan kode mod sendiri):

- Library pihak ketiga yang ikut di `src/`: `utility/subprocess.hpp`, `quickjspp.hpp`, header Mono, dan
  `cloakwork.h`. Data font dan banner (`fonts/`, `textures/header.cpp`, `textures/connect.cpp`).
- Internal Astra UI selain `bool_option`, `canvas::tick`, polling settings, `store_mgr`, dan antrean notifikasi.
- Pengukuran FPS langsung di game. Semua temuan performa berasal dari analisis kode.
