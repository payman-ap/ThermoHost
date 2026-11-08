# Recreate original explanatory figures. No board measurements are used.
# Run from any directory with Windows PowerShell or PowerShell on Windows.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$script:Ink = [System.Drawing.ColorTranslator]::FromHtml('#17324d')
$script:Muted = [System.Drawing.ColorTranslator]::FromHtml('#526579')
$script:Blue = [System.Drawing.ColorTranslator]::FromHtml('#dcecfb')
$script:Green = [System.Drawing.ColorTranslator]::FromHtml('#d9f0e6')
$script:Gold = [System.Drawing.ColorTranslator]::FromHtml('#fff0c7')
$script:Purple = [System.Drawing.ColorTranslator]::FromHtml('#e9e3f5')
$script:Font = [System.Drawing.Font]::new('Segoe UI', 23)
$script:Small = [System.Drawing.Font]::new('Segoe UI', 18)
$script:TitleFont = [System.Drawing.Font]::new('Segoe UI', 32, [System.Drawing.FontStyle]::Bold)

function New-Figure([int]$Width, [int]$Height, [string]$Title) {
    $script:Bitmap = [System.Drawing.Bitmap]::new($Width, $Height)
    $script:Graphics = [System.Drawing.Graphics]::FromImage($script:Bitmap)
    $script:Graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $script:Graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $script:Graphics.Clear([System.Drawing.Color]::White)
    Write-Label $Title 50 30 ($Width - 100) 70 $script:TitleFont
}
function Write-Label([string]$Text, [float]$X, [float]$Y, [float]$Width = 1200,
                     [float]$Height = 65, $TextFont = $script:Font) {
    $brush = [System.Drawing.SolidBrush]::new($script:Ink)
    $rect = [System.Drawing.RectangleF]::new($X, $Y, $Width, $Height)
    $script:Graphics.DrawString($Text, $TextFont, $brush, $rect)
    $brush.Dispose()
}
function Draw-Box([float]$X, [float]$Y, [float]$Width, [float]$Height,
                  [string]$Text, $Fill = $script:Blue) {
    $brush = [System.Drawing.SolidBrush]::new($Fill)
    $pen = [System.Drawing.Pen]::new($script:Ink, 2)
    $script:Graphics.FillRectangle($brush, $X, $Y, $Width, $Height)
    $script:Graphics.DrawRectangle($pen, $X, $Y, $Width, $Height)
    $boxFont = if ($Width -lt 120) { $script:Small } else { $script:Font }
    Write-Label $Text ($X + 18) ($Y + 12) ($Width - 32) ($Height - 18) $boxFont
    $brush.Dispose(); $pen.Dispose()
}
function Draw-Segment([float]$X1, [float]$Y1, [float]$X2, [float]$Y2,
                      [bool]$Arrow = $false, $Color = $script:Ink, [float]$Thickness = 3) {
    $pen = [System.Drawing.Pen]::new($Color, $Thickness)
    if ($Arrow) { $pen.CustomEndCap = [System.Drawing.Drawing2D.AdjustableArrowCap]::new(5, 6) }
    $script:Graphics.DrawLine($pen, $X1, $Y1, $X2, $Y2)
    $pen.Dispose()
}
function Save-Figure([string]$Name) {
    $script:Bitmap.Save((Join-Path $PSScriptRoot $Name), [System.Drawing.Imaging.ImageFormat]::Png)
    $script:Graphics.Dispose(); $script:Bitmap.Dispose()
    Write-Output "Generated $Name"
}

New-Figure 1200 1290 'One measurement, several contracts'
$boxes = @(
    @('ThermoConsole.exe', 'Windows client: display and stop policy'),
    @('ThermoDevice.dll', 'Your protocol, handles, parser, time budget'),
    @('Winusb.dll / winusb.sys', 'Microsoft API and kernel function driver'),
    @('USB host stack and controllers', 'Host schedules OUT commands and IN replies'),
    @('RP2040 + TinyUSB', 'Firmware: command, conversion wait, response'),
    @('MAX6675 + thermocouple', 'SPI word from a physical measurement')
)
for ($i = 0; $i -lt $boxes.Count; $i++) {
    $y = 125 + 175 * $i
    $fill = if ($i -ge 4) { $script:Green } else { $script:Blue }
    Draw-Box 100 $y 1000 125 ($boxes[$i][0] + "`n" + $boxes[$i][1]) $fill
    if ($i -lt 5) {
        Draw-Segment 545 ($y + 128) 545 ($y + 168) $true
        Draw-Segment 650 ($y + 168) 650 ($y + 128) $true
    }
}
Write-Label 'Conceptual responsibilities; lower USB internals are simplified.' 100 1200 1000 60 $script:Small
Save-Figure 'architecture.png'

New-Figure 1400 850 'A fresh read: illustrative timeline'
Write-Label 'Configured conversion wait: 250 ms. Other widths are illustrative.' 50 115 1300 60
$stages = @(
    @(110, 165, 'Host', 'Request'),
    @(275, 150, 'USB OUT', 'Transport'),
    @(425, 610, 'RP2040 / MAX6675', 'Restart + conversion wait (250 ms)'),
    @(1035, 80, 'SPI', 'Read'),
    @(1115, 175, 'USB IN', 'Reply')
)
$fills = @($script:Blue, $script:Purple, $script:Green, $script:Gold, $script:Purple)
for ($i = 0; $i -lt $stages.Count; $i++) {
    $s = $stages[$i]
    Draw-Box $s[0] 260 $s[1] 150 $s[2] $fills[$i]
    Write-Label $s[3] $s[0] 440 $s[1] 150 $script:Small
}
Draw-Segment 110 620 1290 620 $true
Write-Label 'Time advances; drawing is not to scale' 350 650 900 60
Write-Label 'Host scheduling and parsing add latency. A timeout is not a hard deadline.' 70 750 1250 70 $script:Small
Save-Figure 'read-timeline.png'

New-Figure 1400 890 'The 32-byte response'
Write-Label 'Offsets are decimal. Every small cell represents one byte.' 50 115 1300 60
$fields = @(
    @(0, 4, 'THR2 magic', $script:Blue),
    @(4, 1, 'Flags', $script:Gold),
    @(5, 3, 'Reserved', $script:Gold),
    @(8, 4, 'Temperature x100', $script:Green),
    @(12, 4, 'Uptime ms', $script:Purple),
    @(16, 16, 'Token echoed from request', $script:Blue)
)
foreach ($f in $fields) {
    for ($j = 0; $j -lt $f[1]; $j++) {
        $offset = $f[0] + $j
        $row = [math]::Floor($offset / 16)
        $col = $offset % 16
        Draw-Box (60 + 80 * $col) (245 + 180 * $row) 80 80 ([string]$offset) $f[3]
    }
}
Write-Label '0-3: magic    4: flags    5-7: reserved (zero)' 65 610 1270 55
Write-Label '8-11: signed temperature x100    12-15: unsigned uptime in ms' 65 670 1270 55
Write-Label '16-31: sixteen opaque token bytes; numeric fields use little endian' 65 730 1270 80
Save-Figure 'response-layout.png'

New-Figure 1400 850 'SPI read: conceptual clock and data'
Write-Label 'Selection and sixteen clock positions; not a captured waveform.' 50 115 1300 60
$left = 180; $step = 65
Write-Label 'CS' 60 240 100 70
Write-Label 'SCK' 50 420 120 70
Write-Label 'SO' 60 590 100 70
Draw-Segment 180 240 220 240
Draw-Segment 220 240 220 300
Draw-Segment 220 300 1260 300
Draw-Segment 1260 300 1260 240
Draw-Segment 1260 240 1330 240
Draw-Segment 180 480 220 480
for ($i = 0; $i -lt 16; $i++) {
    $x = 220 + $i * $step
    Draw-Segment $x 480 $x 420
    Draw-Segment $x 420 ($x + 32) 420
    Draw-Segment ($x + 32) 420 ($x + 32) 480
    Draw-Segment ($x + 32) 480 ($x + $step) 480
    $raw = 0x0330
    $bit = ($raw -shr (15 - $i)) -band 1
    Draw-Box $x 590 $step 65 ([string]$bit) $script:Green
}
Write-Label 'Example bits: raw 0x0330 -> 102 counts -> 25.50 C' 180 720 1150 60
Write-Label 'Bit positions D15 to D0 run left to right. Conversion occurs with CS high.' 100 785 1250 60 $script:Small
Save-Figure 'spi.png'

New-Figure 1400 1000 'Live firmware: application states'
Draw-Box 450 160 500 120 "Waiting`nAssemble a THQ2 command" $script:Blue
Draw-Box 450 410 500 120 "Converting`nService USB; check deadline" $script:Green
Draw-Box 450 680 500 120 "Sending`nKeep partial-write progress" $script:Gold
Draw-Segment 700 280 700 405 $true
Write-Label 'Complete request: copy token, pulse CS, set deadline' 780 300 570 100 $script:Small
Draw-Segment 700 535 700 675 $true
Write-Label 'Deadline reached: SPI read and encode response' 780 565 570 100 $script:Small
Draw-Segment 450 740 280 740
Draw-Segment 280 740 280 220
Draw-Segment 280 220 445 220 $true
Write-Label 'All bytes accepted' 45 420 220 100 $script:Small
Write-Label 'New complete commands supersede pending work and restart conversion.' 70 860 1270 60
Write-Label 'Unmount clears command/conversion/response state. Queued bytes are separate.' 70 925 1270 60 $script:Small
Save-Figure 'state-machine.png'

New-Figure 1400 900 'When production outruns consumption'
Write-Label 'Illustrative unbounded FIFO: 4 produced/s, 1 consumed/s.' 50 115 1300 60
$plotLeft = 180; $plotTop = 220; $plotWidth = 1050; $plotHeight = 470
for ($i = 0; $i -le 3; $i++) {
    $y = $plotTop + $plotHeight - ($i / 3) * $plotHeight
    Draw-Segment $plotLeft $y ($plotLeft + $plotWidth) $y $false ([System.Drawing.Color]::LightGray) 1
    Write-Label ([string]($i * 10)) 110 ($y - 20) 60 50 $script:Small
}
Draw-Segment $plotLeft ($plotTop + $plotHeight) ($plotLeft + $plotWidth + 30) ($plotTop + $plotHeight) $true
Draw-Segment $plotLeft ($plotTop + $plotHeight) $plotLeft ($plotTop - 30) $true
Draw-Segment $plotLeft ($plotTop + $plotHeight) ($plotLeft + $plotWidth) $plotTop $false ([System.Drawing.Color]::SeaGreen) 6
foreach ($t in @(0, 2, 4, 6, 8, 10)) {
    $x = $plotLeft + ($t / 10) * $plotWidth
    Write-Label ([string]$t) ($x - 12) 700 70 50 $script:Small
}
Write-Label 'Queued samples' 230 175 500 50 $script:Small
Write-Label 'Elapsed time (seconds)' 500 760 650 60
Write-Label 'Real buffers are finite: blocking or loss changes this model at capacity.' 65 830 1280 60 $script:Small
Save-Figure 'queue-growth.png'

$script:Font.Dispose(); $script:Small.Dispose(); $script:TitleFont.Dispose()
