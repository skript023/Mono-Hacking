#include <windows.h>
#include <cstdio>
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    for (int i = 0; i < 10; ++i) {
        HMODULE module = LoadLibraryA(argv[1]);
        if (!module) return 3;
        auto exercise = reinterpret_cast<int (*)()>(GetProcAddress(module, "exercise_tls"));
        if (!exercise || !exercise()) return 4;
        if (!FreeLibrary(module)) return 5;
        if (GetModuleHandleA(argv[1])) {
            std::fprintf(stderr, "TLS DLL remains loaded after FreeLibrary (cycle %d)\n", i);
            return 1;
        }
    }
    std::puts("PASS: TLS DLL unloaded and reinitialized across 10 cycles");
}
