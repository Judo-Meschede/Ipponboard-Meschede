param(
    [Parameter(Mandatory=$true)][string]$IconPath,
    [Parameter(Mandatory=$true)][string]$PngPath
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::new(128, 128)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$black = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(12, 13, 18))
$white = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
$blue = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(5, 25, 231))
$yellow = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 241, 45))
$red = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(245, 29, 33))
$fontTitle = [System.Drawing.Font]::new('Arial', 10, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
$fontScore = [System.Drawing.Font]::new('Arial', 56, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
$fontTimer = [System.Drawing.Font]::new('Arial', 31, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
$center = [System.Drawing.StringFormat]::new()
$center.Alignment = [System.Drawing.StringAlignment]::Center
$center.LineAlignment = [System.Drawing.StringAlignment]::Center
try {
    # The same compact scoreboard is used in the executable resource and every window.
    $graphics.FillRectangle($black, 0, 0, 128, 128)
    $graphics.DrawString('Ipponboard-Meschede', $fontTitle, $yellow, [System.Drawing.RectangleF]::new(2, 7, 124, 19), $center)
    $graphics.FillRectangle($white, 5, 32, 58, 61)
    $graphics.FillRectangle($blue, 65, 32, 58, 61)
    $graphics.DrawString('0', $fontScore, $black, [System.Drawing.RectangleF]::new(5, 27, 58, 70), $center)
    $graphics.DrawString('0', $fontScore, $white, [System.Drawing.RectangleF]::new(65, 27, 58, 70), $center)
    $graphics.DrawString('4:00', $fontTimer, $red, [System.Drawing.RectangleF]::new(0, 94, 128, 32), $center)
    $graphics.Flush()
    $bitmap.Save($PngPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $icon = [System.Drawing.Icon]::FromHandle($bitmap.GetHicon())
    $stream = [System.IO.File]::Create($IconPath)
    try { $icon.Save($stream) }
    finally { $stream.Dispose(); $icon.Dispose() }
}
finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    foreach ($obj in @($black,$white,$blue,$yellow,$red,$fontTitle,$fontScore,$fontTimer,$center)) { $obj.Dispose() }
}
