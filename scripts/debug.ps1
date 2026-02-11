$ErrorActionPreference = "Stop"

Write-Host "=== DEBUG SCRIPT STARTED ==="
Write-Host "PWD: $(Get-Location)"

$root = "C:\Users\hammi\development\PixelWorldCpp"
$exe  = "C:\Users\hammi\development\PixelWorldCpp\godot-executable-4-4\Godot_v4.4.1-stable_win64_console.exe"
$proj = "C:\Users\hammi\development\PixelWorldCpp\project"

if (!(Test-Path $exe)) {
    Write-Error "Godot executable NOT FOUND: $exe"
}

if (!(Test-Path $proj)) {
    Write-Error "Project path NOT FOUND: $proj"
}

Write-Host "=== Killing running Godot (if any) ==="
Get-Process Godot_v4.4.1-stable_mono_win64 -ErrorAction SilentlyContinue | Stop-Process -Force

Set-Location $root

Write-Host "=== Building GDExtension (Debug) ==="
scons dev_build=yes

if ($LASTEXITCODE -ne 0) {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show(
        "SCons build failed. Godot will NOT launch.",
        "Build Error",
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::None
    )

    Write-Host "=== SCons failed, NOT launching Godot ==="
    exit $LASTEXITCODE
}

Write-Host "=== Launching Godot ==="
Start-Process -FilePath $exe -ArgumentList "--path `"$proj`"" -PassThru

Write-Host "=== DEBUG SCRIPT FINISHED ==="

