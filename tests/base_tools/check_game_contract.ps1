param([Parameter(Mandatory=$true)][string]$ManagedPath)
$ErrorActionPreference = 'Stop'
$contracts = @{
    Smelter = @('double GetDeltaTime\(\)', 'float GetFuel\(\)', 'int GetQueueSize\(\)')
    Fermenter = @('double GetFermentationTime\(\)', 'float m_fermentationDuration', 'Empty,\s+Fermenting,\s+Exposed,\s+Ready')
    Beehive = @('float GetTimeSinceLastUpdate\(\)', 'int GetHoneyLevel\(\)', 'int m_maxHoney')
    Plant = @('float GetGrowTime\(\)', 'double TimeSincePlanted\(\)', 'void UpdateHealth\(double ', 'GameObject Grow\(\)', 'Healthy,\s+NoSun')
    EnvMan = @('void FixedUpdate\(\)', 'string GetEnvironmentOverride\(\)', 'bool m_debugTimeOfDay', 'float m_debugTime', 'List<EnvSetup> m_environments')
    Container = @('void StackAll\(\)', 'void RPC_StackResponse\(long \w+, bool \w+\)')
    Inventory = @('bool AddItem\(ItemDrop.ItemData item\)', 'int StackAll\(Inventory fromInventory, bool message = false\)')
    WearNTear = @('bool Repair\(\)', 'float GetHealthPercentage\(\)')
    SE_Rested = @('List<Piece> GetNearbyComfortPieces\(Vector3 point\)')
    Piece = @('int GetComfort\(\)', 'ComfortGroup m_comfortGroup')
    ItemDrop = @('Consumable = 2', 'Ammo = 9', 'AmmoNonEquipable = 23')
}
foreach ($entry in $contracts.GetEnumerator()) {
    $source = (& ilspycmd -t $entry.Key (Join-Path $ManagedPath 'assembly_valheim.dll')) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw "Decompilation failed: $($entry.Key)" }
    foreach ($pattern in $entry.Value) {
        if ($source -notmatch $pattern) { throw "Game contract mismatch: $($entry.Key): $pattern" }
    }
    if ($entry.Key -eq 'Inventory') {
        $singleArgument = [regex]::Matches($source, '(?m)^\s*(?:public|private) \w+(?:\.\w+)* AddItem\([^,\r\n]*\)\s*$')
        if ($singleArgument.Count -ne 1) { throw 'Inventory.AddItem one-argument overload is ambiguous' }
    }
    Write-Output "PASS $($entry.Key)"
}
$source = (& ilspycmd -t 'UnityEngine.Object' (Join-Path $ManagedPath 'UnityEngine.CoreModule.dll')) -join "`n"
if ($LASTEXITCODE -ne 0 -or $source -notmatch 'Object\[\] FindObjectsOfType\(Type type\)') { throw 'Unity object scan signature changed' }
Write-Output 'PASS UnityEngine.Object scan overload'
