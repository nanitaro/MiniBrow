# Makes the release archive release\MiniBrow-<version>.zip (files under a MiniBrow\ folder; the version is
# MB_VERSION in version.h).
# Run after "wmake -h" (the VS Code task does both). ASCII only (Windows PowerShell 5.1).
# The zip entries use "/" as the separator: Compress-Archive of PowerShell 5.1 writes "\", which some
# unzip tools turn into flat file names.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$files = @('minibrow.exe', 'minibrow.ini', 'README.txt', 'LICENSE.txt', 'THIRD_PARTY_LICENSES.txt',
           'xs\licenses\lgpl-3.0.txt', 'xs\licenses\gpl-3.0.txt', 'xs\licenses\apache-2.0.txt',
           'xs\licenses\NOTICE', 'xs\licenses\readme.md')
foreach ($f in $files) {
  if (-not (Test-Path -LiteralPath (Join-Path $root $f))) { throw "missing: $f" }
}

# the version: MB_VERSION must match MB_VER_MAJOR.MB_VER_MINOR (the numbers go into the exe's version info)
$vh = [IO.File]::ReadAllText((Join-Path $root 'version.h'), [Text.Encoding]::GetEncoding(932))
$mVer = [regex]::Match($vh, '(?m)^#define\s+MB_VERSION\s+"([0-9.]+)"')
$mMaj = [regex]::Match($vh, '(?m)^#define\s+MB_VER_MAJOR\s+(\d+)')
$mMin = [regex]::Match($vh, '(?m)^#define\s+MB_VER_MINOR\s+(\d+)')
if (-not ($mVer.Success -and $mMaj.Success -and $mMin.Success)) { throw 'version.h: MB_VERSION / MB_VER_MAJOR / MB_VER_MINOR not found' }
$version = $mVer.Groups[1].Value
if ($version -ne ($mMaj.Groups[1].Value + '.' + $mMin.Groups[1].Value)) {
  throw "version.h: MB_VERSION ""$version"" does not match MB_VER_MAJOR.MB_VER_MINOR ($($mMaj.Groups[1].Value).$($mMin.Groups[1].Value))"
}

# the ini must hold the shipping values (the working copy may carry this PC's proxy or log path)
$ini = [IO.File]::ReadAllLines((Join-Path $root 'minibrow.ini'), [Text.Encoding]::GetEncoding(932))
$want = @{ 'Proxy' = ''; 'JSLog' = ''; 'UserAgent' = ''; 'ProxyPlainHttps' = '0' }
foreach ($k in $want.Keys) {
  $line = $ini | Where-Object { $_ -match "^$k=" } | Select-Object -First 1
  if ($null -eq $line) { throw "minibrow.ini: $k= is missing" }
  $v = $line.Substring($k.Length + 1).Trim()
  if ($v -ne $want[$k]) { throw "minibrow.ini: $k=$v (the release needs $k=$($want[$k]))" }
}

# the exe must be the BearSSL build: the wolfSSL build (GPLv3) is never distributed
$exe = Join-Path $root 'minibrow.exe'
$exeText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
if ($exeText.Contains('wolfSSL') -or $exeText.Contains('wolfCrypt')) { throw 'minibrow.exe contains wolfSSL: build it with "wmake -h" (not USE_WOLFSSL=1)' }
$newest = Get-ChildItem $root -File | Where-Object { $_.Extension -in '.c', '.h', '.inc', '.rc' -or $_.Name -eq 'minibrow.ini' } |
          Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($newest -and $newest.LastWriteTime -gt (Get-Item $exe).LastWriteTime) {
  throw "minibrow.exe is older than $($newest.Name): build it first"
}

$outDir = Join-Path $root 'release'
New-Item -ItemType Directory -Force $outDir | Out-Null
$zip = Join-Path $outDir ('MiniBrow-' + $version + '.zip')
if (Test-Path -LiteralPath $zip) { [IO.File]::Delete($zip) }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
try {
  foreach ($f in $files) {
    $name = 'MiniBrow/' + $f.Replace('\', '/')
    [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, (Join-Path $root $f), $name,
      [IO.Compression.CompressionLevel]::Optimal) | Out-Null
  }
} finally {
  $archive.Dispose()
}
"created $zip ($((Get-Item $zip).Length) bytes)"
foreach ($f in $files) { '  MiniBrow\' + $f }
