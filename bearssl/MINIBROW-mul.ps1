param([string]$Src = (Join-Path $PSScriptRoot 'src'))
# MiniBrow: Open Watcom makes every 64-bit multiplication and 64-bit shift a library call (__U8M / __U8LS / __U8RS / __I8RS).
# BearSSL's i31 / m31 code (RSA, X25519, P-256) multiplies 32x32->64 and shifts 64-bit values all the time.
# On a Pentium 100 one RSA-4096 public operation took about 650 ms, X25519 about 79 ms, P-256 about 198 ms.
#  part 1 - inner.h: MUL() and MUL31() as one MUL instruction (#pragma aux), BR_SHR31() = 64-bit shift right by 31;
#           int/i31_montmul.c: the shifts of the Montgomery multiplication use BR_SHR31()
#  part 2 - inner.h: BR_SHR64 (SHRD + SHR), br_mb_sar64 (SHRD + SAR), BR_DBL64 (ADD + ADC);
#           ec/ec_c25519_m31.c and ec/ec_p256_m31.c: the 64-bit shifts use them (the arithmetic shift ARSHW,
#           "w >> 30", "w >> 15", "(MUL31(..)) << 1"; "(y << n) & 0x3FFFFFFF" only needs the low 32 bits of y)
# The results are the same values (other compilers keep the original C expressions).
# Byte-preserving (Latin-1). Writes nothing unless every replacement of a part matches. ASCII only.
$enc = [Text.Encoding]::GetEncoding(28591)
function Read-Src([string]$rel) { return [IO.File]::ReadAllText((Join-Path $Src $rel), $enc) }
function Write-Src([string]$rel, [string]$text) { [IO.File]::WriteAllText((Join-Path $Src $rel), $text, $enc) }
function Edit-Exact([string]$text, [string]$old, [string]$new, [int]$count = 1) {
  $n = ([regex]::Matches($text, [regex]::Escape($old))).Count
  if ($count -ge 0 -and $n -ne $count) { throw "expected $count match(es), found $n : $($old.Substring(0, [Math]::Min(80, $old.Length)))" }
  if ($n -lt 1) { throw "not found: $($old.Substring(0, [Math]::Min(80, $old.Length)))" }
  return $text.Replace($old, $new)
}

$inner = Read-Src 'inner.h'
$nl = if ($inner.Contains("`r`n")) { "`r`n" } else { "`n" }
function Fix-Newlines([string]$t) { return $t.Replace("`r`n", "`n").Replace("`n", $nl) }

# ---- part 1
if (-not $inner.Contains('br_mb_mul')) {
  $mont = Read-Src 'int\i31_montmul.c'
  $oldMul = Fix-Newlines "#define MUL(x, y)   ((uint64_t)(x) * (uint64_t)(y))`n"
  $newMul = Fix-Newlines @"
#if defined(__WATCOMC__)
/* MiniBrow: Open Watcom makes every 64-bit multiplication and shift a library call (__U8M / __U8RS).
   The 32x32->64 product and the 64-bit shift right by 31 as single instructions (same values). */
uint64_t br_mb_mul(uint32_t x, uint32_t y);
#pragma aux br_mb_mul = "mul edx" parm [eax] [edx] value [edx eax] modify exact [eax edx];
uint64_t br_mb_shr31(uint64_t z);
#pragma aux br_mb_shr31 = "shrd eax,edx,31" "shr edx,31" parm [eax edx] value [edx eax] modify exact [eax edx];
#define MUL(x, y)   br_mb_mul((uint32_t)(x), (uint32_t)(y))
#define BR_SHR31(z) br_mb_shr31(z)
#else
#define MUL(x, y)   ((uint64_t)(x) * (uint64_t)(y))
#define BR_SHR31(z) ((z) >> 31)
#endif

"@
  $inner = Edit-Exact $inner $oldMul $newMul
  $oldM31 = '#define MUL31(x, y)     ((uint64_t)(x) * (uint64_t)(y))'
  $inner = Edit-Exact $inner $oldM31 '#define MUL31(x, y)     MUL(x, y)' -1
  $mont = Edit-Exact $mont 'r = z >> 31;' 'r = BR_SHR31(z);' 5
  $mont = Edit-Exact $mont 'dh = zh >> 31;' 'dh = BR_SHR31(zh);'
  Write-Src 'inner.h' $inner
  Write-Src 'int\i31_montmul.c' $mont
  'part 1 patched'
} else { 'part 1 already patched' }

# ---- part 2
$inner = Read-Src 'inner.h'
if (-not $inner.Contains('br_mb_sar64')) {
  $c255 = Read-Src 'ec\ec_c25519_m31.c'
  $p256 = Read-Src 'ec\ec_p256_m31.c'
  $oldDef = Fix-Newlines "#define MUL(x, y)   br_mb_mul((uint32_t)(x), (uint32_t)(y))`n#define BR_SHR31(z) br_mb_shr31(z)`n#else`n#define MUL(x, y)   ((uint64_t)(x) * (uint64_t)(y))`n#define BR_SHR31(z) ((z) >> 31)`n#endif`n"
  $newDef = Fix-Newlines @"
#define MUL(x, y)   br_mb_mul((uint32_t)(x), (uint32_t)(y))
#define BR_SHR31(z) br_mb_shr31(z)
/* 64-bit shifts by a count below 32 (logical / arithmetic) and doubling, as single instructions */
uint64_t br_mb_shr64(uint64_t z, unsigned n);
#pragma aux br_mb_shr64 = "shrd eax,edx,cl" "shr edx,cl" parm [eax edx] [ecx] value [edx eax] modify exact [eax edx];
uint64_t br_mb_sar64(uint64_t z, unsigned n);
#pragma aux br_mb_sar64 = "shrd eax,edx,cl" "sar edx,cl" parm [eax edx] [ecx] value [edx eax] modify exact [eax edx];
uint64_t br_mb_dbl64(uint64_t z);
#pragma aux br_mb_dbl64 = "add eax,eax" "adc edx,edx" parm [eax edx] value [edx eax] modify exact [eax edx];
#define BR_SHR64(z, n) br_mb_shr64((z), (n))
#define BR_DBL64(z)    br_mb_dbl64(z)
#else
#define MUL(x, y)   ((uint64_t)(x) * (uint64_t)(y))
#define BR_SHR31(z) ((z) >> 31)
#define BR_SHR64(z, n) ((uint64_t)(z) >> (n))
#define BR_DBL64(z)    ((uint64_t)(z) << 1)
#endif

"@
  $inner = Edit-Exact $inner $oldDef $newDef

  # the 15 doubled sums of products of square9() in both files: ((MUL31(a[i], a[j]) + MUL31(..) ...) << 1)
  $rxDbl = [regex]'\(\((MUL31\(a\[\d\], a\[\d\]\)(?:\s*\+\s*MUL31\(a\[\d\], a\[\d\]\))*)\) << 1\)'
  foreach ($name in 'c255', 'p256') {
    $t = if ($name -eq 'c255') { $c255 } else { $p256 }
    $n = $rxDbl.Matches($t).Count
    if ($n -ne 15) { throw "$name : expected 15 doubled products, found $n" }
    $t = $rxDbl.Replace($t, 'BR_DBL64($1)')
    # carries: "cc = w >> 30;" (64-bit w in mul9 / square9 / reductions; for a 32-bit w the value is the same)
    $t = Edit-Exact $t 'cc = w >> 30;' 'cc = BR_SHR64(w, 30);' -1
    if ($name -eq 'c255') { $c255 = $t } else { $p256 = $t }
  }
  $c255 = Edit-Exact $c255 'cc = MUL31(w >> 15, 19);' 'cc = MUL31(BR_SHR64(w, 15), 19);' 2
  $c255 = Edit-Exact $c255 'cc = MUL31((uint32_t)(w >> 15), 19);' 'cc = MUL31((uint32_t)BR_SHR64(w, 15), 19);'

  # P-256: arithmetic shift of 64-bit values (every use adds to / assigns an unsigned 64-bit value, so the bits are the same)
  $oldAr = Fix-Newlines "#else`n#define ARSH(x, n)    ((*(int32_t *)&(x)) >> (n))`n#define ARSHW(x, n)   ((*(int64_t *)&(x)) >> (n))`n#endif"
  $newAr = Fix-Newlines "#elif defined(__WATCOMC__)`n#define ARSH(x, n)    ((*(int32_t *)&(x)) >> (n))`n#define ARSHW(x, n)   br_mb_sar64((x), (n))`n#else`n#define ARSH(x, n)    ((*(int32_t *)&(x)) >> (n))`n#define ARSHW(x, n)   ((*(int64_t *)&(x)) >> (n))`n#endif"
  $p256 = Edit-Exact $p256 $oldAr $newAr
  # (y << n) & 0x3FFFFFFF with a 64-bit y: only the low 32 bits of y matter (n <= 28, mask below 2^30)
  $rxLo = [regex]'\(y << (\d+)\) & 0x3FFFFFFF'
  $n = $rxLo.Matches($p256).Count
  if ($n -ne 8) { throw "p256 : expected 8 masked shifts of y, found $n" }
  $p256 = $rxLo.Replace($p256, '(uint64_t)(((uint32_t)y << $1) & 0x3FFFFFFF)')

  Write-Src 'inner.h' $inner
  Write-Src 'ec\ec_c25519_m31.c' $c255
  Write-Src 'ec\ec_p256_m31.c' $p256
  'part 2 patched'
} else { 'part 2 already patched' }
