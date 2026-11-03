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
    # Multi-resolution ICO instead of the old single-size GetHicon() format.
    # Windows Explorer, the taskbar and RC compiler can now choose an exact size.
    $sizes = @(16, 24, 32, 48, 64, 128, 256)
    $images = [System.Collections.Generic.List[byte[]]]::new()
    foreach ($size in $sizes) {
        $scaled = [System.Drawing.Bitmap]::new($size, $size)
        $canvas = [System.Drawing.Graphics]::FromImage($scaled)
        $canvas.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $canvas.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $canvas.DrawImage($bitmap, [System.Drawing.Rectangle]::new(0,0,$size,$size))
        $canvas.Dispose()
        $buffer = [System.IO.MemoryStream]::new()
        try {
            $scaled.Save($buffer, [System.Drawing.Imaging.ImageFormat]::Png)
            $images.Add($buffer.ToArray())
        } finally {
            $buffer.Dispose()
            $scaled.Dispose()
        }
    }
    $stream = [System.IO.File]::Create($IconPath)
    $binary = [System.IO.BinaryWriter]::new($stream)
    try {
        $binary.Write([uint16]0)       # ICONDIR reserved
        $binary.Write([uint16]1)       # ICO type
        $binary.Write([uint16]$sizes.Count)
        $offset = 6 + $sizes.Count * 16
        for ($i = 0; $i -lt $sizes.Count; $i++) {
            $size = $sizes[$i]
            $binary.Write([byte]($size % 256))
            $binary.Write([byte]($size % 256))
            $binary.Write([byte]0)
            $binary.Write([byte]0)
            $binary.Write([uint16]1)
            $binary.Write([uint16]32)
            $binary.Write([uint32]$images[$i].Length)
            $binary.Write([uint32]$offset)
            $offset += $images[$i].Length
        }
        foreach ($png in $images) {
            $binary.Write([byte[]]$png)
        }
    } finally { $binary.Dispose() }
    # Check the output structure before letting a Windows build use it.
    $testStream = [System.IO.File]::OpenRead($IconPath)
    try {
        $testReader = [System.IO.BinaryReader]::new($testStream)
        if ($testReader.ReadUInt16() -ne 0 -or $testReader.ReadUInt16() -ne 1 -or
            $testReader.ReadUInt16() -ne $sizes.Count) {
            throw 'Ungültiges mehrstufiges Windows-Icon.'
        }
    } finally { $testReader.Dispose() }
}
finally {
    $graphics.Dispose()
    $bitmap.Dispose()
    foreach ($obj in @($black,$white,$blue,$yellow,$red,$fontTitle,$fontScore,$fontTimer,$center)) { $obj.Dispose() }
}
