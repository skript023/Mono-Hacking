// API: main_batch.add/run/fullscan_run/dispatch.
// This pattern is illustrative; replace it with a signature verified against
// the target binary before scanning.

main_batch.add("typescript_example_scan", "48 8B ?? ?? ??", (address) => {
  console.log("Signature result:", address);
});

// Scan the named module. Omit the argument or pass null/"" to scan the default module.
main_batch.run("GameAssembly.dll");

function beginFullScan(): void {
  main_batch.fullscan_run();
}

function dispatchScanCallbacks(): void {
  // Call after the asynchronous full scan has had time to finish.
  main_batch.dispatch();
}

// Full-scan helpers are defined but not started automatically by this sample.

