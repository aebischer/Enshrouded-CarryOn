param([string]$Output = "$PSScriptRoot/../build/pincer_pixels.h")
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$bitmap=[System.Drawing.Bitmap]::new("$PSScriptRoot/../assets/item/transport-pincer.png")
try {
 if($bitmap.Width -ne 200 -or $bitmap.Height -ne 200){throw 'Expected 200x200 icon'}
 $lines=[System.Collections.Generic.List[string]]::new()
 $lines.Add('#pragma once')
 $lines.Add('inline constexpr unsigned char PincerPixels[] = {')
 for($y=0;$y -lt 200;$y++) {
  $row=[System.Collections.Generic.List[string]]::new()
  for($x=0;$x -lt 200;$x++){$c=$bitmap.GetPixel($x,$y);$row.Add("$($c.R),$($c.G),$($c.B),$($c.A)")}
  $lines.Add(($row -join ',')+',')
 }
 $lines.Add('};')
 [IO.File]::WriteAllLines([IO.Path]::GetFullPath($Output),$lines)
} finally {$bitmap.Dispose()}
