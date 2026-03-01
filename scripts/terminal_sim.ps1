# Kill any existing terminal_sim.exe processes
Get-Process -Name "terminal_sim" -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction Silence

Write-Host "Building terminal_sim.exe..."
clang++ -std=c++17 -DNOMINMAX -I./src -o ./scripts/terminal_sim.exe ./src/terminal_main.cpp
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host "Running terminal simulation..." -ForegroundColor Green
.\scripts\terminal_sim.exe
