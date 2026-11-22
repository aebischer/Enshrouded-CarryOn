param(
 [string]$Fixtures=(Join-Path $PSScriptRoot '..\..\work'),
 [string]$ModelReference=(Join-Path $PSScriptRoot '..\..\work\enshrouded-blender-reference\enshrouded_tools\core'),
 [string]$PackageValidator=(Join-Path $PSScriptRoot '..\..\build\validate_mod_package.exe')
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Fixtures=(Resolve-Path -LiteralPath $Fixtures).Path
$files=@{
 compoundBench='resource-18070f4c1f54d144a971e9d7fbaa114f-0x39768775-0.bin'
 compoundBed='resource-1d1ac3c7dd6f3e4f9ba46b961cb5e2fe-0x39768775-0.bin'
 sitHelper='resource-9965cff6b1cc7246a918a4d2fd22210e-0x39768775-0.bin'
 sleepHelper='resource-a2eec50bf05ef944b132d2adc6945397-0x39768775-0.bin'
 bed='resource-4b6fcb124a52de42aad04d0d767761d3-0x39768775-0.bin'
 chair='resource-e3d4d0024ff04d46a6f5077534be1d24-0x39768775-0.bin'
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
$files['chest_0']='resource-85a49dbc6516fa46aa45c40911719925-0x39768775-0.bin'
$files['chest_1']='resource-2b8d25353e39044d9392bd990be5ae61-0x39768775-0.bin'
$files['chest_2']='resource-a9d6ebbd599e654cb056ad0070db97dc-0x39768775-0.bin'
$files['chest_3']='resource-8a7dddb0691e1845b52fe94cfa769ebc-0x39768775-0.bin'
$files['chest_4']='resource-2ea6e7fa70bc644db55a8d222c152f3a-0x39768775-0.bin'
$files['chest_5']='resource-2fa35b95ea084d41bd98ac75990aef04-0x39768775-0.bin'
$files['chest_6']='resource-90a8ba652a024b459cb7b92cb8c06cd4-0x39768775-0.bin'
$files['chest_7']='resource-f12592431d195c41a335013419b202e7-0x39768775-0.bin'
$files['chest_8']='resource-35b87dd454fefd44b5b3d6c340673c41-0x39768775-0.bin'
$files['chest_9']='resource-7e7e660cbd0fb94cb58dc3a20df7eaec-0x39768775-0.bin'
$files['chest_10']='resource-bd4c83bcf1584a4997fb0dc0248eb738-0x39768775-0.bin'
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
