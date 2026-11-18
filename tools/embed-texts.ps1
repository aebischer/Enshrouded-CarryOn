$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot
$catalog=Get-Content -Raw -Encoding UTF8 "$root/assets/localization/texts.json" | ConvertFrom-Json
$keys=@('item.name','item.caption','item.description','setting.move_chests','setting.move_objects','setting.move_interactive','setting.inspect_objects','inspect.target','inspect.model','transport.enter','transport.exit','transport.grab','transport.place','transport.finish')
& "$root/sdk/tools/compile-translations.ps1" -Catalog "$root/assets/localization/texts.json" -Output "$root/build/carryon_texts.h" -Symbol CarryOnTexts
$manifest=[Collections.Generic.List[string]]::new()
Get-Content -Encoding UTF8 "$root/src/carryon.dll.mod" | ForEach-Object {$manifest.Add($_)}
foreach($locale in $catalog.PSObject.Properties) {
 if($locale.Name -notmatch '^(en|fr|de|it|es|ru|pl|ja|ko|pt-br|th|tr|zh-cn|zh-tw|uk)$'){throw 'Unsupported language code'}
 $actual=@($locale.Value.PSObject.Properties.Name)
 if(@(Compare-Object $keys $actual).Count -ne 0){throw "Incomplete catalogue: $($locale.Name)"}
 foreach($key in $keys) {
  $value=$locale.Value.$key
  $invalidControls=if($key -eq 'item.description'){'[\x00-\x09\x0b-\x1f|=]'}else{'[\x00-\x1f|=]'}
  if($value -isnot [string] -or [string]::IsNullOrWhiteSpace($value) -or $value -match $invalidControls -or [Text.Encoding]::UTF8.GetByteCount($value) -gt 1024){throw "Invalid text: $key"}
  if($key.StartsWith('setting.')) {
   if([Text.Encoding]::UTF8.GetByteCount($value) -gt 100){throw "Setting label too long: $key"}
   $manifest.Add('text='+$locale.Name+'|'+$key+'|'+$value)
  }
 }
}
$utf8=[Text.UTF8Encoding]::new($false)
[IO.File]::WriteAllLines("$root/dist/Mods/carryon/carryon.dll.mod",$manifest,$utf8)
Write-Output ('Validated {0} languages and {1} text keys.' -f @($catalog.PSObject.Properties).Count,$keys.Count)

