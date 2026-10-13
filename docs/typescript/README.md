# Dokumentasi TypeScript

Dokumentasi API scripting TypeScript untuk Valheim Hacks. Runtime menjalankan JavaScript melalui QuickJS; file `.ts` adalah sumber yang dikompilasi menjadi `.js` sebelum dijalankan. Referensi ini mengikuti binding native di `src/services/javascript/bindings/`.

## Daftar isi

- [Alur kerja](#alur-kerja)
- [Kompilasi dan instalasi skrip](#kompilasi-dan-instalasi-skrip)
- [Tipe dan pointer Mono](#tipe-dan-pointer-mono)
- [API global](#api-global)
- [API `mono`](#api-mono)
- [Modul `detour`](#modul-detour)
- [Modul `canvas`](#modul-canvas)
- [Command dan penyimpanan](#command-dan-penyimpanan)
- [Signature scanner](#signature-scanner)
- [Contoh](#contoh)
- [Batasan runtime](#batasan-runtime)

## Alur kerja

1. Buat atau edit skrip TypeScript.
2. Kompilasikan ke JavaScript dengan TypeScript compiler (`tsc`).
3. Letakkan hasil `.js` di folder `scripts` di direktori kerja aplikasi.
4. Salin satu file `.js` yang ingin dijalankan ke root folder `scripts` di direktori kerja aplikasi. Buka Javascript Console → Script Manager untuk memuat dan menjalankannya. Daftar di Script Manager hanya memindai file `.js` langsung di root `scripts`; pemuatan modul internal juga memindai subfolder secara rekursif.

QuickJS tidak mengeksekusi TypeScript secara langsung. Tipe dari file deklarasi hanya membantu editor dan compiler; tipe tersebut tidak menambah fungsi runtime.

## Kompilasi dan instalasi skrip

Pasang TypeScript di lingkungan pengembangan, lalu dari root repositori jalankan:

```powershell
npx.cmd --yes --package typescript tsc --target ES2020 --module ES2020 --lib ES2020 --skipLibCheck --noEmitOnError --outDir .\scripts .\docs\typescript\api.d.ts .\docs\typescript\detour.d.ts .\docs\typescript\valheim_mono.ts
```

Untuk skrip yang mengimpor modul `detour`, sertakan `detour.d.ts` dan entry TypeScript tersebut:

```powershell
npx.cmd --yes --package typescript tsc --target ES2020 --module ES2020 --lib ES2020 --skipLibCheck --noEmitOnError --outDir .\scripts .\docs\typescript\api.d.ts .\docs\typescript\detour.d.ts .\docs\typescript\valheim_achievement_hook.ts
```

Gunakan `--module ES2020` jika skrip memakai `import`. Setelan `--target ES2020` dipilih agar sintaks hasil kompilasi sesuai dengan runtime QuickJS proyek. Skrip tanpa import juga boleh dikompilasi satu per satu. Hasil contoh pertama adalah `scripts/valheim_mono.js`.

`api.d.ts` dan `detour.d.ts` adalah deklarasi editor/compiler, bukan berkas yang perlu dipasang ke runtime. Untuk menjalankan skrip dari Script Manager, letakkan file `.js` langsung di root `scripts`. File di jalur yang mengandung `includes` dilewati loader modul; file `.js` di subfolder lain dapat dimuat saat pemuatan modul, meskipun tidak muncul di daftar Script Manager.

## Tipe dan pointer Mono

Banyak binding mengembalikan pointer native/Mono sebagai `number` JavaScript karena lapisan QuickJS mengekspos alamat melalui `double`. Perlakukan nilai ini sebagai token opaque: teruskan kembali ke binding yang memintanya, jangan dihitung sebagai alamat aritmetika. Nilai `0` berarti null/tidak ditemukan. Jangan memanggil API dengan pointer nol atau pointer dari tipe yang salah.

Nama assembly dan class/method harus cocok dengan build game yang sedang berjalan. Binding `mono` tidak menginferensi namespace, overload berdasarkan tipe parameter, atau status static/instance. Cek signature aktual pada assembly game.

## API global

### `console`

Semua argumen diformat native sebagai teks dan digabung dengan spasi. Keluaran masuk ke log aplikasi dan Javascript Console.

- `console.log(...values)` — tingkat info.
- `console.warn(...values)` — tingkat peringatan.
- `console.error(...values)` — tingkat error.

Binding ini mengubah argumen menjadi string; jangan mengandalkan inspeksi objek seperti devtools.

### `logger`

- `logger.info(message: string): void` — tulis satu pesan tingkat info.

### `unity`

Fungsi berikut mengembalikan pointer Mono sebagai angka:

- `unity.get_local_player(): number` — objek pemain lokal.
- `unity.get_zone_system(): number` — objek `ZoneSystem`.
- `unity.get_env_man(): number` — objek `EnvMan`.

Contoh:

```ts
const player = unity.get_local_player();
if (player === 0) {
  console.warn("Pemain lokal belum tersedia");
}
```

### `Vector3`

`new Vector3(x, y, z)` membuat nilai vektor native dengan properti numerik `x`, `y`, dan `z`. Properti adalah akses nilai; binding tidak menyediakan setter JavaScript untuk mengubah komponen.

```ts
const point = new Vector3(12.5, 45, -9.8);
console.log(point.x, point.y, point.z);
```

### `commands`

Diimpor sebagai modul: `import { call } from "commands";`

- `call(name: string): void` — cari command native dengan nama tersebut dan jalankan jika ditemukan. Nama diubah ke hash internal (joaat); command yang tidak ditemukan diabaikan.

### `store`

Objek store native yang juga dipakai binding GUI. Semua operasi memakai key string; persistensi mengikuti implementasi store native.

- `get_bool(key: string): boolean`
- `set_bool(key: string, value: boolean): void`
- `get_int(key: string): number`
- `set_int(key: string, value: number): void`
- `get_float(key: string): number`
- `set_float(key: string, value: number): void`

Nilai integer/float dikonversi ke tipe native yang sesuai. Gunakan key unik untuk menghindari tabrakan dengan menu atau skrip lain.

## API `mono`

`mono` tersedia sebagai global (`mono.get_class(...)`) dan juga sebagai named export modul (`import { mono } from "mono"`). Modul juga mengekspos class `Mono` dengan method static yang sama. Fungsi di bawah mengembalikan pointer/nilai native dalam bentuk `number`, kecuali disebutkan lain.

| Fungsi | Kegunaan |
| --- | --- |
| `get_root_domain(): number` | Pointer root domain Mono. |
| `get_class(className: string, assembly: string): number` | Cari class berdasarkan nama class dan assembly. Namespace tidak dapat diberikan lewat binding ini. |
| `get_method(className: string, methodName: string, parameterCount: number, assembly: string): number` | Cari method berdasarkan class, nama, jumlah parameter, dan assembly. Tidak memilih overload berdasarkan tipe parameter. |
| `get_field(classPointer: number, fieldName: string): number` | Cari field dari pointer class. |
| `invoke_method(methodPointer: number, objectPointer: number, ...args: (string \| number \| boolean)[]): number` | Panggil method Mono dan kembalikan pointer `MonoObject` hasil (atau `0` untuk hasil null/void). Untuk method instance, `objectPointer` adalah objek target; untuk method static, gunakan target yang sesuai dengan runtime Mono. |
| `set_field_int(objectPointer: number, fieldPointer: number, value: number): void` | Tulis field integer 32-bit pada objek. |
| `set_field_float(objectPointer: number, fieldPointer: number, value: number): void` | Tulis field float 32-bit pada objek. |
| `set_field_string(objectPointer: number, fieldPointer: number, value: string): void` | Binding tersedia, tetapi saat ini meneruskan pointer C string ke `mono_field_set_value` tanpa membuat `MonoString`. Jangan pakai untuk mengisi field `System.String`. |
| `get_field_value(objectPointer: number, fieldPointer: number): number` | Salin field ke satu slot pointer native lalu kembalikan slot itu sebagai `number`. Ini bukan pembacaan bertipe: reference dapat tampak sebagai pointer; nilai primitif tampil sebagai bit mentah (misalnya float bukan angka float). Jangan pakai untuk struct yang lebih besar dari slot pointer. |
| `get_field_offset(fieldPointer: number): number` | Offset byte field dari awal instance. |
| `get_static_field_value(className: string, fieldName: string): number` | Kembalikan alamat slot field static (`static data + field offset`), bukan isi field. Helper mencari class di assembly `Assembly-CSharp` dan namespace kosong; assembly/namespace tidak dapat dipilih dari binding ini. |
| `get_static_field_data(classPointer: number): number` | Pointer area data static untuk class. |
| `get_static_field_data_vtable(vtablePointer: number): number` | Pointer area data static untuk vtable. |
| `get_vtable(classPointer: number): number` | Ambil vtable class. |
| `get_class_from_method(methodPointer: number): number` | Ambil class pemilik method. |
| `get_compile_method(className: string, methodName: string, parameterCount: number, assembly: string): number` | Cari alamat kode hasil kompilasi method, untuk dipakai misalnya oleh `Hook.add`. |
| `unbox(objectPointer: number): number` | Kembalikan alamat data unbox dari boxed value. Pointer berlaku selama objek/data backing masih valid. |
| `unbox_vec3(objectPointer: number): Vector3 \| null` | Salin `Vector3` dari boxed Mono object menjadi objek `Vector3`; null jika objek/data tidak tersedia. |

`invoke_method` menerima semua argumen JavaScript sebagai string terlebih dahulu. Binding mengenali teks `true`/`false`, lalu mencoba parse integer, lalu angka pecahan; teks lain dibuat menjadi `MonoString`. Akibatnya, string yang tampak numerik seperti `"123"` diteruskan sebagai integer, bukan string. Tidak ada metadata tipe parameter: method dengan tipe C# lain dapat salah ditafsirkan. Parameter `ref`/`out`, array, enum, struct, generic, dan overload ambigu tidak otomatis ditangani. Untuk method static, verifikasi target object yang sesuai pada runtime Mono sebelum invoke.

Contoh pencarian aman:

```ts
const klass = mono.get_class("Player", "assembly_valheim");
if (klass === 0) {
  console.error("Class Player tidak ditemukan");
} else {
  const field = mono.get_field(klass, "m_baseHP");
  console.log("class", klass, "field", field);
}
```

Binding saat ini tidak menyediakan pembacaan field bertipe (`get_field_int`/`get_field_float` tidak ada). `get_field_value` hanya aman untuk field yang muat dalam satu slot pointer; hasil float adalah pola bit yang dibaca sebagai integer, bukan konversi ke float. `set_field_string` juga belum mengonversi ke `MonoString`, jadi jangan gunakan untuk field string managed.

Untuk static field, `get_static_field_value` memberi alamat slot. API JavaScript ini tidak menyediakan dereference bertipe terhadap alamat tersebut. Pakai `get_static_field_data`, `get_vtable`, dan offset hanya jika memahami layout memory native.

## Modul `detour`

Gunakan named import: `import { Hook } from "detour";`. Modul juga mengekspor `NativeContext` dan `NativeArgument` sebagai deklarasi tipe.

### `Hook`

- `Hook.add(name: string, address: number, argumentCount: number, callback: (context: NativeContext) => unknown): boolean` — buat hook pada alamat native. `false` bila alamat/callback tidak valid, nama sudah terdaftar, jumlah argumen lebih dari 256, atau pemasangan gagal. Hook yang baru didaftarkan belum aktif.
- `Hook.enable(name: string): void` — aktifkan hook terdaftar. Nama tidak dikenal diabaikan.
- `Hook.disable(name: string): void` — hapus hook dan pulihkan target jika aktif. Hook dengan nama tidak dikenal diabaikan.

`argumentCount` harus sesuai ABI fungsi native yang di-hook, termasuk `this` implisit pada method instance. Indeks argumen dimulai dari nol. Callback berjalan di jalur detour native; jaga tetap singkat dan jangan lakukan operasi yang tidak aman untuk thread pemanggil.

### `NativeContext`

- `args(): number[]` — nilai mentah dari slot integer/pointer untuk setiap indeks. Untuk empat posisi argumen pertama, nilai floating point disimpan di register XMM terpisah dan tidak tercermin di hasil `args()`; gunakan `getArg(i).float()` atau `.double()` sesuai signature.
- `getArg(index: number): NativeArgument` — akses argumen ABI.
- `setArg(index: number, value: number): void` — ganti argumen register integer/pointer.
- `setFloat(index: number, value: number): void` — ganti argumen float.
- `setDouble(index: number, value: number): void` — ganti argumen double.
- `callOriginal(): number` — panggil fungsi asli paling banyak sekali per invocation; pemanggilan berikutnya mengembalikan hasil tersimpan.

### `NativeArgument`

- `number(): number` / `valueOf(): number` — nilai integer/pointer mentah.
- `float(): number` — baca argumen 32-bit floating point.
- `double(): number` — baca argumen 64-bit floating point.
- `string(): string` — baca pointer argumen sebagai narrow C string null-terminated tanpa konversi encoding; hanya gunakan untuk parameter yang memang C string.
- `monoString(): string \| null` — konversi pointer `MonoString` UTF-16 ke string UTF-8; mengembalikan null untuk pointer null atau panjang string di luar batas binding. Pointer harus tetap valid.

Jika callback mengembalikan angka, angka itu diubah menjadi `uintptr_t` dan menjadi return di register integer. Return floating point di register XMM tidak didukung oleh binding callback ini. Jika callback mengembalikan nilai nonangka atau melempar exception, dispatcher memakai hasil `callOriginal()` (memanggil fungsi asli jika belum dipanggil). `callOriginal()` juga mengembalikan nilai integer/pointer mentah; jangan gunakan pola ini untuk mengubah return float/double.

Contoh pass-through:

```ts
import { Hook } from "detour";

const address = mono.get_compile_method("Achievements", "CanGetAchievements", 1, "assembly_valheim");
if (address !== 0) {
  const name = "Achievements.CanGetAchievements";
  const ok = Hook.add(name, address, 1, (ctx) => {
    console.log("cheated arg:", ctx.getArg(0).number());
    return ctx.callOriginal();
  });
  if (ok) Hook.enable(name);
}
```

Contoh memakai method yang juga dicari pada `src/hooking.cpp`; hook native di proyek memakai satu parameter `bool`. Karena itu `argumentCount` adalah satu dan argumen nol adalah `cheated` (tidak ada `this`). Tetap verifikasi alamat dan signature pada versi game yang dijalankan.

## Modul `canvas`

Gunakan `import * as canvas from "canvas";` atau `import { add_tab, add_submenu } from "canvas";`. Callback pembuatan menerima `MenuBuilder`.

- `add_tab(title: string, id: number, build: (menu: MenuBuilder) => void): void` — daftarkan tab menu dengan ID numerik.
- `add_submenu(title: string, id: number, build: (menu: MenuBuilder) => void): void` — API pendaftaran submenu tersedia. Pada implementasi saat ini versi build submenu baru tidak dinaikkan, sementara renderer melewati menu dengan versi build/render sama; akibatnya submenu baru biasanya tidak muncul.

`MenuBuilder` menyediakan:

- `add_bool(text: string, description?: string): void` — toggle bool; nilai disimpan melalui store dengan key yang sama dengan `text`.
- `add_slider(text: string, min: number, max: number, step: number): void` — slider integer jika min/max/step semuanya integer, selain itu slider pecahan. Builder tidak menerima callback perubahan; implementasi membaca nilai awal dari store tetapi tidak menulis nilai slider kembali ke store.
- `add_choose(text: string, values: string[], callback: (index: number, value: string) => void): void` — pilihan string. Index callback dimulai dari **1**.
- `add_option(text: string, callback?: () => void): void` — item yang menjalankan callback saat dipilih.
- `add_submenu(text: string, submenuId: number, callback?: () => void): void` — tautan ke submenu ID; callback opsional dijalankan saat opsi dipilih.

Contoh API builder (catatan: pendaftaran submenu eksternal di bawah belum dirender oleh binding saat ini):

```ts
import * as canvas from "canvas";

canvas.add_tab("Contoh", 45001, (menu) => {
  menu.add_bool("Aktif", "Contoh toggle");
  menu.add_slider("Jarak", 1, 100, 1);
  menu.add_choose("Mode", ["Aman", "Cepat"], (index, value) => {
    console.log("Pilihan", index, value);
  });
  menu.add_option("Cetak pesan", () => console.log("Dipilih"));
  menu.add_submenu("Lanjutan", 45002);
});

canvas.add_submenu("Lanjutan", 45002, (menu) => {
  menu.add_bool("Opsi lanjutan");
});
```

Gunakan ID menu unik. Nama opsi dipakai sebagai key store untuk toggle dan nilai awal slider; nama yang sama dapat berbagi state. Toggle menulis state ke store saat diubah.

## Command dan penyimpanan

### Modul `commands`

```ts
import { call } from "commands";
call("nama_command");
```

Untuk membuat command baru gunakan `command_types`:

```ts
import { CommandFactory } from "command_types";

const toggle = CommandFactory.create_bool("contoh_toggle", "Contoh toggle", "Deskripsi", false);
toggle.on_enable(() => console.log("aktif"));
toggle.on_disable(() => console.log("nonaktif"));
toggle.set_state(true);

const loop = CommandFactory.create_looped("contoh_loop", "Contoh loop", "Berjalan tiap tick");
loop.on_tick(() => console.log("tick"));
loop.on_disable(() => console.log("loop dihentikan"));
loop.set_state(true);
```

`BoolCommand` dan `LoopedCommand` menyediakan `get_state(): boolean` serta `set_state(state: boolean): void`. Callback pendaftaran bersifat pengganti callback sebelumnya. `BoolCommand` memiliki `on_enable` dan `on_disable`; `LoopedCommand` memiliki `on_tick` dan `on_disable`. `create_bool(name, label, description, defaultState)` dan `create_looped(name, label, description)` mengembalikan objek command.

### Signature scanner `main_batch`

`main_batch` adalah objek global:

- `main_batch.add(name: string, signature: string, callback: (address: number) => void): void` — daftarkan signature dan callback hasil.
- `main_batch.run(moduleName?: string | null): void` — scan batch pada module yang disebutkan; argumen kosong/null/string kosong memakai module default proses. Callback hasil dipanggil saat hasil didispatch.
- `main_batch.fullscan_run(): void` — jadwalkan full scan di thread pool. Setelah selesai, panggil `main_batch.dispatch()` untuk mengirim hasil ke callback JavaScript.
- `main_batch.dispatch(): void` — proses hasil scan yang sudah diantrikan.

Contoh:

```ts
main_batch.add("contoh", "48 8B ?? ?? ??", (address) => {
  console.log("signature:", address);
});
main_batch.run("GameAssembly.dll");
```

Format signature mengikuti parser native memory pattern. Pastikan modul dan pola sesuai binary yang sedang berjalan. Hasil scan adalah alamat mentah; scan sendiri tidak memasang hook.

## Contoh

Lihat [`valheim_mono.ts`](./valheim_mono.ts) untuk akses global dan pemeriksaan pointer, serta [`valheim_achievement_hook.ts`](./valheim_achievement_hook.ts) untuk pola detour pass-through.

Folder [`examples`](./examples/) berisi contoh terpisah per binding:

- `01_globals_unity_vector.ts` — console, logger, Unity globals, dan `Vector3`.
- `02_mono_lookup.ts` — pencarian class/method/field, alamat method, vtable, dan data static.
- `03_mono_fields.ts` — baca nilai mentah dan pola menulis field integer, float, dan string.
- `04_mono_invoke_unbox.ts` — invoke method dan unbox object/`Vector3`.
- `05_detour.ts` — registrasi, aktivasi, pass-through, dan pelepasan hook.
- `06_canvas.ts` — tab, submenu, dan semua jenis kontrol menu.
- `07_commands_store.ts` — pemanggilan/pembuatan command dan operasi store.
- `08_signature_scan.ts` — batch scan, full scan, dan dispatch callback.

Kompilasi satu contoh ke folder output, misalnya:

```powershell
npx.cmd --yes --package typescript tsc --target ES2020 --module ES2020 --lib ES2020 --skipLibCheck --noEmitOnError --outDir .\scripts .\docs\typescript\api.d.ts .\docs\typescript\detour.d.ts .\docs\typescript\examples\05_detour.ts
```

Untuk contoh yang tidak memakai `detour`, `detour.d.ts` boleh dihilangkan dari perintah. Contoh yang memodifikasi field atau memasang hook memiliki operasi tersebut dalam helper/pola eksplisit; periksa alamat, tipe field, dan signature target sebelum memanggilnya.

### Build semua contoh

Setiap file contoh adalah entry script mandiri. Perintah PowerShell berikut membangun delapan file di `examples` dan dua contoh utama secara terpisah, supaya deklarasi global antarfile tidak bertabrakan. Hasilnya masuk ke `build/typescript`; pilih dan salin hanya file `.js` yang ingin dijalankan ke folder `scripts`.

```powershell
$outputDir = ".\build\typescript"
$sourceFiles = @(Get-ChildItem ".\docs\typescript\examples" -Filter *.ts) + @(Get-Item ".\docs\typescript\valheim_mono.ts", ".\docs\typescript\valheim_achievement_hook.ts")

foreach ($sourceFile in $sourceFiles) {
  npx.cmd --yes --package typescript tsc --target ES2020 --module ES2020 --lib ES2020 --skipLibCheck --noEmitOnError --outDir $outputDir ".\docs\typescript\api.d.ts" ".\docs\typescript\detour.d.ts" $sourceFile.FullName
  if ($LASTEXITCODE -ne 0) {
    throw "TypeScript build failed: $($sourceFile.Name)"
  }
}
```

Status build: delapan file di `examples` dan dua contoh utama berhasil dikompilasi menjadi 10 file JavaScript dengan TypeScript 7.0.2. Hasil pemeriksaan ini ditulis ke direktori sementara, bukan ke folder skrip aktif.

## Batasan runtime

- Binding adalah API native langsung dan tidak memvalidasi seluruh tipe/umur pointer. Pointer kedaluwarsa, nama salah, atau signature yang tidak sesuai dapat menyebabkan crash.
- `get_method` dan `get_compile_method` memilih method berdasarkan jumlah parameter saja. Pastikan overload-nya benar secara terpisah.
- Hasil dan argumen pointer memakai `number`, bukan tipe pointer aman; jangan lakukan aritmetika alamat di JavaScript.
- Tidak semua tipe C# dapat dikonversi oleh `invoke_method`; API ini bukan refleksi bertipe penuh.
- Jangan mengasumsikan daftar ini mencakup semua API Unity atau Mono. Yang didokumentasikan adalah ekspor binding JavaScript yang terdaftar oleh proyek.
