# Audit Bug Valheim (Mono-Hacking)

Tanggal: 2026-10-21. Dicek pada branch `shared`, commit `6651dda`.

Cara cek: kode mod dibaca lalu dibandingkan dengan IL `assembly_valheim.dll` dari game yang terpasang
(`E:\SteamLibrary\steamapps\common\Valheim`), lewat runtime Mono milik game. Nilai default game diambil
dari konstruktor field di build sekarang. Nilai yang disimpan di prefab Unity tidak bisa dibaca dengan cara
ini; temuan yang bergantung pada nilai prefab ditandai **belum dipastikan**. Belum ada yang dites di dalam game.

## Ringkasan

| # | Masalah | Dampak | Lokasi |
| --- | --- | --- | --- |
| 1 | Max Eitr ditimpa game setiap detik, eitr ikut terpotong | Tinggi | `features/max_mana.cpp` |
| 2 | Mematikan No Wave & Collision Damage mengisi nilai yang salah | Sedang | `features/ship.cpp` |
| 3 | Ashlands Ocean Boiling Immunity tidak bisa dimatikan | Sedang | `features/ship.cpp` |
| 4 | Ship Speed Boost memakai angka tetap untuk semua kapal | Sedang, belum dipastikan | `features/ship.cpp` |
| 5 | Mematikan Disable Base Raids mengisi peluang raid yang salah | Rendah | `features/world.cpp` |
| 6 | Mematikan fitur fishing mengisi nilai yang salah | Rendah | `features/fishing.cpp` |
| 7 | Never Wet berlaku untuk semua karakter dan juga menghapus efek Tar | Sedang | `hooks/update_water.cpp` |
| 8 | Enam opsi menu tidak dipakai kode mana pun | Sedang | beberapa file |
| 9 | Label Food Duration Multiplier terbalik dengan perilakunya | Rendah | `menu/submenu/player_menu.cpp` |
| 10 | Max Body Armor menyala secara default dengan nilai 999 | Info | `hooks/get_body_armor.cpp` |
| 11 | Enam hook berisiko tidak terpanggil karena inline JIT | Sedang, belum dipastikan | `hooking.cpp` |
| 12 | Speed fermenter dan tanaman bergantung pada jarak pemain | Rendah | `hooks/base_tools.cpp` |
| 13 | Noclip macet setelah mati atau ganti world, dan melempar exception setiap frame | Sedang | `hooks/player_update.cpp` |
| 14 | Infinite Durability mengisi durability 1000, bukan nilai maksimal item | Sedang | `hooks/durability.cpp` |
| 15 | Makanan dan mead dari slot food tidak pernah berkurang | Tinggi | `unity/equipment_slots.cpp` |
| 16 | `HaveEmptySlot` selalu true untuk semua inventory, loot peti kapal bisa hilang | Sedang | `hooks/have_empty_slot.cpp` |
| 17 | Inventory Size menyembunyikan item saat dimatikan dan membuat item jatuh saat spawn tanpa mod | Sedang | `features/inventory.cpp` |
| 18 | Hook kosong dan log yang tidak perlu | Rendah | beberapa file |

Temuan performa ada di bagian [Performa](#performa).

Sudah diperbaiki sebelumnya: Attack Animation Speed menimpa `Animator.speed` setiap frame walaupun
pengalinya 1.0, sehingga tempo serangan dan hit-stop vanilla hilang. Diperbaiki di `557a25e`.

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

### 2. Mematikan No Wave & Collision Damage mengisi nilai yang salah

- Lokasi: [ship.cpp:68-97](../src/features/ship.cpp#L68-L97).
- Penyebab: `on_disable` mengisi angka tetap: `m_waterImpactDamage = 1` dan `m_upsideDownDmg = 10`.
  Default game adalah `10` dan `20`.
- Akibat: setelah fitur dimatikan, kapal menerima damage 10x lebih kecil dari ombak dan benturan, dan 2x
  lebih kecil saat terbalik, sampai objek kapal dimuat ulang.
- Saran: rekam nilai asli per kapal saat pertama kali ditimpa, lalu kembalikan nilai itu.

### 3. Ashlands Ocean Boiling Immunity tidak bisa dimatikan

- Lokasi: [ship.cpp:10-31](../src/features/ship.cpp#L10-L31).
- Penyebab: `m_ashlandsReady` diset `true` setiap tick, dan class ini tidak punya `on_disable`.
- Akibat: kapal tetap kebal terhadap air Ashlands setelah fitur dimatikan, sampai objek kapal dimuat ulang.
- Saran: rekam nilai asli per kapal dan kembalikan di `on_disable`.

### 4. Ship Speed Boost memakai angka tetap untuk semua kapal (belum dipastikan)

- Lokasi: [ship.cpp:102-166](../src/features/ship.cpp#L102-L166).
- Penyebab: `m_sailForceFactor`, `m_stearForce`, `m_backwardForce`, dan `m_rudderSpeed` ditulis sebagai
  `0.1 / 0.5 / 50 / 0.5` dikali pengali, dan angka yang sama dipakai di `on_disable`. Angka ini sama dengan
  default di konstruktor, tetapi rakit, karve, longship, dan drakkar bisa punya nilai sendiri di prefab.
- Akibat: kalau nilai prefab berbeda, handling kapal berubah walaupun pengali 1x, dan tetap berubah setelah
  fitur dimatikan. Hanya kapal yang sedang dikemudikan yang dipulihkan; kapal yang ditinggal saat fitur
  menyala tetap memakai nilai mod.
- Saran: rekam nilai asli per kapal, kalikan nilai itu, dan kembalikan saat dimatikan.

### 5. Mematikan Disable Base Raids mengisi peluang raid yang salah

- Lokasi: [world.cpp:43](../src/features/world.cpp#L43).
- Penyebab: `on_disable` mengisi `m_eventChance = 20`. Default game adalah `25`.
- Akibat: peluang raid tetap lebih rendah dari vanilla sampai world dimuat ulang. Hanya berpengaruh di host,
  karena `RandEventSystem` memutuskan raid di sana.
- Saran: rekam nilai asli sebelum diisi 0, lalu kembalikan nilai itu.

### 6. Mematikan fitur fishing mengisi nilai yang salah

- Lokasi: [fishing.cpp:50](../src/features/fishing.cpp#L50) dan
  [fishing.cpp:107](../src/features/fishing.cpp#L107).
- Penyebab: Unbreakable Fishing Line mengisi `m_breakDistance = 30` (default `4`). Zero Stamina Fishing
  mengisi `m_pullStaminaUse = 15` (default `10`).
- Akibat: senar hampir tidak bisa putus dan tarikan memakan stamina 50% lebih banyak, tetapi hanya pada
  pelampung yang sedang dilempar saat fitur dimatikan. Lemparan berikutnya membuat pelampung baru.
- Saran: rekam nilai asli per pelampung, atau biarkan pelampung yang sedang aktif tanpa diubah.

### 7. Never Wet berlaku untuk semua karakter dan juga menghapus efek Tar

- Lokasi: [update_water.cpp:6-33](../src/hooks/update_water.cpp#L6-L33).
- Penyebab: hook `Character.UpdateWater` tidak mengecek pemain lokal, dan mengenolkan `m_waterLevel` serta
  `m_tarLevel` sebelum fungsi asli berjalan.
- Akibat: semua karakter yang dikontrol client Anda (musuh, hewan jinak) ikut kebal efek Wet dan Tarred.
  Pemain juga kebal Tarred, padahal nama fiturnya hanya menyebut basah. Berenang tidak terpengaruh, karena
  status renang memakai `m_cashedInLiquidDepth` dan `m_swimTimer`.
- Saran: tambahkan cek `_this == unity::get_local_player()`, lalu putuskan apakah Tar memang ikut dicegah
  (dan sesuaikan nama atau deskripsinya).

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

### 9. Label Food Duration Multiplier terbalik dengan perilakunya

- Lokasi: [player_menu.cpp:526](../src/menu/submenu/player_menu.cpp#L526) dan
  [update_food.cpp:32](../src/hooks/update_food.cpp#L32).
- Penyebab: kode mengalikan kecepatan berkurangnya timer makanan. Deskripsinya sudah benar ("Scale how
  quickly food timers decay"), tetapi labelnya berbunyi "Duration".
- Akibat: nilai 2.0 membuat makanan habis dua kali lebih cepat, kebalikan dari yang diharapkan dari label.
- Saran: ganti label menjadi "Food Decay Speed", atau ubah rumusnya supaya 2.0 berarti dua kali lebih awet.

### 10. Max Body Armor menyala secara default dengan nilai 999

- Lokasi: [get_body_armor.cpp:7](../src/hooks/get_body_armor.cpp#L7), menu
  [player_menu.cpp:493](../src/menu/submenu/player_menu.cpp#L493).
- Keterangan: ini pilihan default, bukan kesalahan kode. Di `settings.json` saat audit, opsi ini juga masih
  menyala, jadi armor pemain selalu 999 selama mod aktif.
- Saran: pertimbangkan default mati, supaya perilaku game hanya berubah kalau pemain menyalakannya.

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

### 12. Speed fermenter dan tanaman bergantung pada jarak pemain

- Lokasi: [base_tools.cpp:11-25](../src/hooks/base_tools.cpp#L11-L25) dan
  [base_tools.cpp:190-203](../src/unity/base_tools.cpp#L190-L203).
- Penyebab: `Fermenter.GetFermentationTime` dan `Plant.GetGrowTime` adalah waktu total, bukan delta.
  Pengali hanya dipakai saat pemain berada dalam radius dan client memegang objeknya.
- Akibat: status "siap" bisa kembali menjadi "belum siap" saat pemain keluar dari radius. Smelter, cooking
  station, beehive, dan sap collector tidak kena, karena hook-nya mengalikan delta.
- Saran: terima perilaku ini dan tulis di dokumentasi, atau simpan progres yang sudah dipercepat.

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

### 14. Infinite Durability mengisi durability 1000, bukan nilai maksimal item

- Lokasi: [durability.cpp:8-13](../src/hooks/durability.cpp#L8-L13).
- Penyebab: setiap kali game menguras durability item yang dipakai, mod menulis `m_durability = 1000`.
- Akibat: item yang maksimalnya di bawah 1000 (kebanyakan senjata, alat, dan obor) mendapat durability di
  atas batas. Nilai ini ikut tersimpan di data item, jadi tetap awet setelah fitur dimatikan. Item yang
  maksimalnya di atas 1000 (misalnya lewat Item Overrides) justru turun ke 1000.
- Saran: jangan menulis apa pun dan cukup lewati pengurasan, atau isi dengan `GetMaxDurability` item itu.

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

### 16. `HaveEmptySlot` selalu true untuk semua inventory

- Lokasi: [have_empty_slot.cpp:6-14](../src/hooks/have_empty_slot.cpp#L6-L14).
- Penyebab: hook berisi `if (true) return true;`, tanpa mengecek setting, `g_running`, atau inventory mana.
- Akibat, berdasarkan pemanggil di game:
  - `Piece.DropResources`: untuk piece yang loot-nya dimasukkan ke peti (`m_destroyedLootPrefab`, misalnya
    kapal yang hancur), game membuat peti baru saat peti lama penuh. Dengan hook ini game terus mengisi peti
    yang sama, dan item yang tidak muat hilang karena hasil `AddItem` diabaikan.
  - `StoreGui.UpdateBuyButton`: tombol Buy tetap aktif walaupun inventory penuh. Pembelian tetap aman karena
    koin baru dipotong kalau `AddItem` berhasil.
- Saran: hapus hook ini, atau kaitkan dengan setting yang jelas dan batasi ke inventory pemain lokal.

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

### 18. Hook kosong dan log yang tidak perlu

- [hud_update_food.cpp:5-8](../src/hooks/hud_update_food.cpp#L5-L8): hook `Hud.UpdateFood` hanya memanggil
  fungsi aslinya. Tidak ada efek, tetapi tetap menambah biaya detour setiap frame.
- [have_empty_slot.cpp:10](../src/hooks/have_empty_slot.cpp#L10): `LOG(VERBOSE)` di setiap pemanggilan.
- [start_attack.cpp:217](../src/hooks/start_attack.cpp#L217): `LOG(FATAL) << "No target found within FOV"`
  ditulis setiap kali aimbot menembak tanpa target. Level FATAL di AsyncLogger tidak menghentikan game, jadi
  ini hanya log berlebih, bukan crash.
- Saran: hapus hook kosong, dan turunkan atau hapus log di jalur yang sering dipanggil.

## Performa

Semua kode di bawah berjalan di thread game. Biaya tambahan per frame langsung mengurangi FPS, dan alokasi
objek managed menambah kerja GC Mono, yang menghentikan game sejenak saat berjalan (stutter).

| # | Masalah | Kapan terasa | Lokasi |
| --- | --- | --- | --- |
| P1 | `get_local_player()` tanpa cache, dipanggil di hook yang sangat sering jalan | Selalu, makin berat dengan banyak musuh | `utility/unity.hpp`, banyak hook |
| P2 | Hook stat item membuat 2 objek managed per pemanggilan | Selalu (hotbar), lebih berat saat inventory terbuka | `hooks/item_stat_overrides.cpp` |
| P3 | ESP membaca semua karakter tanpa batas jarak dan semua drop tiap frame | Saat ESP menyala | `worker/entity_worker.cpp` |
| P4 | Marker bangunan rusak memakai 2 panggilan Mono per marker per frame | Saat Mark Damaged Buildings menyala | `unity/base_tools.cpp` |
| P5 | Data creature dikumpulkan tiap frame walaupun HUD inspector mati | Saat crosshair mengarah ke creature | `unity/animal_tools.cpp` |
| P6 | Noclip membaca dan menulis collider tiap frame walaupun mati | Selalu | `hooks/player_update.cpp` |
| P7 | `Hud.get_instance` dicari berdasarkan nama tiap frame | Selalu | `unity/hud.cpp` |
| P8 | Ukuran layar dibaca lewat 2 panggilan Mono tiap frame | Selalu | `worker/main_worker.cpp` |
| P9 | Inventory Size mengubah ukuran UI dan membuat 2 string setiap frame | Saat Inventory Size menyala | `features/inventory.cpp` |
| P10 | Setiap item yang diambil dari slot equipment menyimpan ulang semua halaman | Setiap tembakan kalau ammo di slot equipment | `hooks/equipment_hooks.cpp` |
| P11 | Override cuaca membuat string managed baru setiap frame | Saat cuaca dipaksa | `hooks/base_tools.cpp` |
| P12 | Forsaken Power Always Ready mencari class dan field setiap frame | Saat fitur menyala | `hooks/update_guardian_power.cpp` |

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

### P2. Hook stat item membuat 2 objek managed per pemanggilan

- Lokasi: [item_stat_overrides.cpp:6-32](../src/hooks/item_stat_overrides.cpp#L6-L32), memanggil
  [item_data.cpp:143-180](../src/unity/item_data.cpp#L143-L180).
- Biaya per panggilan: `to_mono_string(name)` membuat string managed, `mono_runtime_invoke` pada
  `TryGetValue` mem-box hasil `bool` menjadi objek baru, lalu string hasilnya dikonversi ke UTF-8.
- Frekuensi: `GetMaxDurability` dipanggil setiap frame untuk ikon hotbar (`HotkeyBar.UpdateIcons`) dan untuk
  setiap item di grid saat inventory terbuka (`InventoryGrid.UpdateGui`). `GetArmor`, `GetBaseBlockPower`,
  dan `GetDeflectionForce` dipanggil saat menghitung armor, block, dan parry.
- Saran: baca dulu jumlah isi `m_customData` (field `_count` pada Dictionary) dan langsung kembalikan nilai
  asli kalau kosong, karena hampir semua item tidak punya override. Simpan string key managed sekali saja.

### P3. ESP membaca semua karakter dan semua drop tiap frame

- Lokasi: [entity_worker.cpp:32-150](../src/worker/entity_worker.cpp#L32-L150).
- Biaya: untuk setiap karakter yang dimuat game, tanpa batas jarak, ESP membaca posisi, titik atas, HP, dan
  nama (konversi string dari Mono) setiap frame. Untuk drop, posisi setiap drop dibaca dulu sebelum disaring
  60 m, lalu `Localization.Localize` dipanggil per drop per frame.
- Saran: tambahkan batas jarak untuk karakter, simpan nama dan level per objek lalu segarkan sekitar sekali
  per detik, dan cache hasil `Localize` berdasarkan nama prefab.

### P4. Marker bangunan rusak memakai `unity::world_to_screen`

- Lokasi: [base_tools.cpp:620-631](../src/unity/base_tools.cpp#L620-L631), memanggil
  [unity.hpp:882-906](../src/utility/unity.hpp#L882-L906).
- Biaya: setiap marker (sampai 200) memanggil `Camera.get_main` dan `WorldToScreenPoint` lewat Mono, dan
  hasil `Vector3`-nya di-box. Totalnya bisa 400 panggilan dan 200 alokasi per frame.
- Saran: pakai `esp_projection::project`, yang sudah dipakai ESP dan hanya berupa hitungan matriks di C++.

### P5. Data creature dikumpulkan walaupun HUD inspector mati

- Lokasi: [animal_tools.cpp:66](../src/unity/animal_tools.cpp#L66).
- Biaya: setiap frame, selama crosshair mengarah ke creature, sekitar 12 nilai dibaca lewat Mono (nama,
  level, HP, posisi, tameness, love points, dan lainnya), tanpa mengecek apakah Show Animal Inspector HUD menyala.
- Saran: keluar lebih awal kalau HUD inspector dan hotkey tame sama-sama mati.

### P6. Noclip membaca dan menulis collider tiap frame walaupun mati

- Lokasi: [player_update.cpp:83](../src/hooks/player_update.cpp#L83).
- Biaya: setiap frame, `GetCollider`, `get_enabled` (hasil di-box), dan `set_enabled` dipanggil, karena
  `active_collider` dikosongkan di akhir setiap frame saat noclip mati.
- Saran: kalau noclip mati dan tidak ada collider yang perlu dipulihkan, langsung keluar.

### P7. `Hud.get_instance` dicari tiap frame

- Lokasi: [hud.cpp:361](../src/unity/hud.cpp#L361) (dan [hud.cpp:60](../src/unity/hud.cpp#L60)).
- Biaya: `mono::get_method` tanpa `static`, jadi daftar method `Hud` ditelusuri berdasarkan nama setiap frame.
- Saran: jadikan `static`, seperti pola di file lain.

### P8. Ukuran layar dibaca tiap frame

- Lokasi: [main_worker.cpp:35-36](../src/worker/main_worker.cpp#L35-L36).
- Biaya: 2 panggilan Mono dan 2 alokasi box per frame. Kecil, tetapi tidak perlu.
- Saran: baca sekali per detik, misalnya di `slow_run`.

### P9. Inventory Size bekerja penuh setiap frame

- Lokasi: [inventory.cpp:88-109](../src/features/inventory.cpp#L88-L109).
- Biaya: setiap frame, semua item di inventory dibaca untuk mencari baris terbawah,
  `InventoryGui.SetInventorySize` dipanggil (mengubah `sizeDelta` RectTransform), class dan field
  `InventoryGui.m_instance` dicari berdasarkan nama tanpa cache, dan `AddUniqueKeyValue` dipanggil dengan 2
  string managed baru.
- Saran: terapkan ukuran hanya saat setting berubah atau saat inventory berganti, bukan setiap frame.

### P10. Item dari slot equipment memicu `save_to_player()` setiap kali diambil

- Lokasi: [equipment_hooks.cpp:111-140](../src/hooks/equipment_hooks.cpp#L111-L140), memanggil
  [equipment_slots.cpp:512](../src/unity/equipment_slots.cpp#L512).
- Biaya: setiap `Inventory.RemoveItem(ItemData, int)` yang mengambil dari inventory equipment langsung
  menyerialisasi semua halaman equipment (ZPackage, `Inventory.Save`, Base64, lalu `m_customData`). Di game,
  method ini dipakai `Attack.UseAmmo`, jadi kalau ammo ditaruh di slot equipment, ini terjadi di setiap
  tembakan. Hal yang sama berlaku saat memasukkan bahan bakar atau bijih satu per satu.
- Saran: tandai sebagai kotor dan simpan sekali per beberapa detik, atau saat game menyimpan (`player_save`).

### P11. Override cuaca membuat string managed baru setiap frame

- Lokasi: [base_tools.cpp:49-56](../src/hooks/base_tools.cpp#L49-L56).
- Biaya: `EnvMan.GetEnvironmentOverride` dipanggil setiap frame, dan selama cuaca dipaksa hook ini memanggil
  `to_mono_string(cfg.weather)` setiap kali. Selain itu, semua hook produksi dan cuaca memanggil
  `get_options()`, yang mengunci mutex dan menyalin seluruh struct opsi termasuk string.
- Saran: simpan string managed cuaca sekali (dengan GC handle) dan buat ulang hanya saat pilihannya berubah.

### P12. Forsaken Power Always Ready mencari class dan field setiap frame

- Lokasi: [update_guardian_power.cpp:9-12](../src/hooks/update_guardian_power.cpp#L9-L12).
- Biaya: `mono::get_class` dan `mono::get_field` dipanggil tanpa `static` di setiap `UpdateGuardianPower`,
  dan `get_local_player()` dihitung sebelum setting dicek.
- Saran: jadikan lookup `static`, dan cek setting lebih dulu.

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
