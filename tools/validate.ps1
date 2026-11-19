param(
 [string]$Fixtures=(Join-Path $PSScriptRoot '..\..\work'),
 [string]$ModelReference=(Join-Path $PSScriptRoot '..\..\work\enshrouded-blender-reference\enshrouded_tools\core'),
 [string]$PackageValidator=(Join-Path $PSScriptRoot '..\..\build\validate_mod_package.exe')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Fixtures=(Resolve-Path -LiteralPath $Fixtures).Path
$files=@{
 door='resource-550d7a506569c348bc33efdf9fb894d4-0x39768775-0.bin'
 props='prop-placement-fixtures.bin'
 bottles='resource-ee386f839f792f4fa64f4a31593a2a1a-0x39768775-0.bin'
 counter='resource-e3a56abbb97f3946ad1fbd266c5ccace-0xb5ce8765-0.bin'
 item='resource-873d587c00f3174da27e9a77fbbfaeba-0xb5ce8765-0.bin'
 items='resource-5dd364304273ca40bdc9aad58f83bf45-0x72ab39b6-0.bin'
 recipes='resource-0fde575e0c2859458191f02c1ba656e2-0x5f5382cf-0.bin'
 ui='resource-f64de1468ffb664f9463e23b9e19e48e-0xfef918b8-0.bin'
 icon='resource-ac11e027e3891e4b8aa6d69aedac1a2d-0xf9e86997-0.bin'
 hammer='resource-618b3ac8dbea284d914688c4c8d9cb98-0x39768775-0.bin'
 wand='resource-0b6db1ac5b832949a2f00c6046bfc35c-0x39768775-0.bin'
}
foreach($key in @($files.Keys)) {
 $files[$key]=Join-Path $Fixtures $files[$key]
 if(!(Test-Path -LiteralPath $files[$key])){throw "Missing native fixture: $($files[$key])"}
}
if(!(Test-Path -LiteralPath $PackageValidator)){throw 'SDK package validator missing'}
if(!(Test-Path -LiteralPath (Join-Path $ModelReference 'render_model.py'))){throw 'Independent KFC4 decoder missing'}
& "$root\build.cmd"
if($LASTEXITCODE -ne 0){throw 'Build/SDK/policy tests failed'}
& "$root\tools\test-content.cmd" $files.items $files.recipes $files.ui
if($LASTEXITCODE -ne 0){throw 'Native content tests failed'}
& "$root\tools\test-native.cmd" $files.item $files.ui $files.icon $files.counter $files.bottles $files.door
if($LASTEXITCODE -ne 0){throw 'Native adapter tests failed'}
& "$root\tools\test-memory.cmd" $files.counter $files.props
if($LASTEXITCODE -ne 0){throw 'Memory/processing regression tests failed'}
& "$root\tools\test-floor.cmd" $files.counter $files.props
if($LASTEXITCODE -ne 0){throw 'Prop corpus placement tests failed'}
$previousFixtures=$env:CARRYON_FIXTURES
$previousReference=$env:CARRYON_MODEL_REFERENCE
try {
 $env:CARRYON_FIXTURES=$Fixtures
 $env:CARRYON_MODEL_REFERENCE=(Resolve-Path -LiteralPath $ModelReference).Path
 & python "$root\tools\test-pincer-model.py"
 if($LASTEXITCODE -ne 0){throw 'Independent model tests failed'}
} finally {
 $env:CARRYON_FIXTURES=$previousFixtures
 $env:CARRYON_MODEL_REFERENCE=$previousReference
}
& $PackageValidator "$root\dist" carryon/carryon.dll
if($LASTEXITCODE -ne 0){throw 'Package validation failed'}
$report=@{time=(Get-Date -Format o);dll=(Get-FileHash "$root\dist\Mods\EnshroudedModLoader\carryon\carryon.dll").Hash;
 manifest=(Get-FileHash "$root\dist\Mods\EnshroudedModLoader\carryon\carryon.dll.mod").Hash;gameplayTested=$false;fixtures=@{}}
foreach($key in $files.Keys){$report.fixtures[$key]=(Get-FileHash -LiteralPath $files[$key]).Hash}
$report | ConvertTo-Json -Depth 3 | Set-Content -Encoding UTF8 "$root\build\validation.json"
Write-Output 'PASS: complete offline validation; no installation and no gameplay automation.'
