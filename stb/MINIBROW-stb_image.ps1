param([string]$File)
# MiniBrow: speed up stb_image's JPEG decoding on a Pentium (slow IMUL; Open Watcom without strict aliasing).
#  part 1 - YCbCr -> RGB with 4 tables of the same products (bit-identical results);
#           optional AAN fast integer IDCT (the method of libjpeg's jidctfst.c), chosen by stbi_mb_fast_idct (1 = fast)
#  part 2 - progressive JPEG: stbi__jpeg_decode_block_prog_ac keeps the bit buffer in local variables in the AC
#           refinement scans (same reading rules; the C code is MINIBROW-progac.txt next to this script); with the fast
#           IDCT, stbi__jpeg_finish dequantizes inside the IDCT (stbi__mb_idct_fast_dq: only nonzero coefficients,
#           no write-back) instead of calling stbi__jpeg_dequantize first.
#           Same results (checked pixel by pixel, also with truncated files). On a Pentium 100 the refinement scans
#           went from 1449 to 1160 ms and dequantize + IDCT from 1354 to 1188 ms for a 1024x575 progressive JPEG.
#  part 4 - reduced-size decoding (stbi_mb_jpeg_shift = 1, 2, 3: 1/2, 1/4, 1/8) with a small IDCT of the low coefficients
#           (stbi__mb_idct_reduce); shift 0 decodes exactly as before. Pentium 100, 1/2 1/4 1/8: baseline 1138 ms ->
#           659 / 440 / 315 ms, progressive 3586 ms -> 2487 / 2246 / 2086 ms (the entropy decoding stays).
#           As in libjpeg 6b, subsampled planes are reduced less (4:2:0 at 1/2: chroma at full size, no upsampling).
#  (there is no part 3: one multiplication per coefficient was tried and gave nothing on a Pentium 100)
# Byte-preserving (Latin-1). Writes nothing unless every replacement of a part matches. ASCII only.
if (-not $File -or -not (Test-Path -LiteralPath $File)) { "stop: -File must be stb_image.h"; exit 2 }
$enc = [Text.Encoding]::GetEncoding(28591)
$s = [IO.File]::ReadAllText($File, $enc)
$nl = if ($s.Contains("`r`n")) { "`r`n" } else { "`n" }
function Edit-Exact([string]$text, [string]$old, [string]$new) {
  $n = ([regex]::Matches($text, [regex]::Escape($old))).Count
  if ($n -ne 1) { throw "expected 1 match, found $n : $($old.Substring(0, [Math]::Min(80, $old.Length)))" }
  return $text.Replace($old, $new)
}
function L([string]$t) { return $t.Replace("`r`n", "`n").Replace("`n", $nl) }

# ---- part 1
if (-not $s.Contains('stbi_mb_fast_idct')) {
# 1. color conversion with tables
$oldColor = L @"
static void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
{
   int i;
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed +  cr* stbi__float2fixed(1.40200f);
      g = y_fixed + (cr*-stbi__float2fixed(0.71414f)) + ((cb*-stbi__float2fixed(0.34414f)) & 0xffff0000);
      b = y_fixed                                     +   cb* stbi__float2fixed(1.77200f);
"@
$newColor = L @"
// MiniBrow: the products below are taken from tables (same values; a Pentium IMUL takes about 10 clocks)
static int stbi__mb_ctab_ready;
static int stbi__mb_cr_r[256], stbi__mb_cr_g[256], stbi__mb_cb_g[256], stbi__mb_cb_b[256];
static void stbi__mb_ctab_init(void)
{
   const int kr = stbi__float2fixed(1.40200f), kg1 = -stbi__float2fixed(0.71414f);
   const int kg2 = -stbi__float2fixed(0.34414f), kb = stbi__float2fixed(1.77200f);
   int i;
   for (i = 0; i < 256; ++i) {
      int c = i - 128;
      stbi__mb_cr_r[i] = c * kr;
      stbi__mb_cr_g[i] = c * kg1;
      stbi__mb_cb_g[i] = (c * kg2) & 0xffff0000;
      stbi__mb_cb_b[i] = c * kb;
   }
   stbi__mb_ctab_ready = 1;     // several threads may fill the tables at once; they write the same values
}

static void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
{
   int i;
   if (!stbi__mb_ctab_ready) stbi__mb_ctab_init();
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 20) + (1<<19); // rounding
      int r,g,b;
      int cr = pcr[i];
      int cb = pcb[i];
      r = y_fixed + stbi__mb_cr_r[cr];
      g = y_fixed + stbi__mb_cr_g[cr] + stbi__mb_cb_g[cb];
      b = y_fixed + stbi__mb_cb_b[cb];
"@
$s = Edit-Exact $s $oldColor $newColor

# 2. fast IDCT and the switch
$oldSetup = L @"
static void stbi__setup_jpeg(stbi__jpeg *j)
{
   j->idct_block_kernel = stbi__idct_block;
"@
$newSetup = L @"
// MiniBrow: AAN fast integer IDCT (the method of libjpeg's jidctfst.c: 5 multiplies per 1-D pass instead of 12).
// stb dequantizes before the IDCT, so the AAN scale factors are applied here to the nonzero coefficients only.
// Pixels may differ by about +-1..2 from stbi__idct_block. MiniBrow sets stbi_mb_fast_idct from minibrow.ini (FastJpeg=).
int stbi_mb_fast_idct = 1;
static const int stbi__mb_aan[64] = {
   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
   22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
   21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
   19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
   12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
    8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
    4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
};
#define STBI__MB_DEQ(c, k)  ((c) ? (((int) (c) * stbi__mb_aan[k] + 2048) >> 12) : 0)
#define STBI__MB_MUL(v, c)  (((v) * (c)) >> 8)
static stbi_uc stbi__mb_clamp(int x)
{
   if ((unsigned) x > 255) return x < 0 ? 0 : 255;
   return (stbi_uc) x;
}
static void stbi__mb_idct_fast(stbi_uc *out, int out_stride, short data[64])
{
   int ws[64], i;
   for (i = 0; i < 8; ++i) {                        // columns
      const short *in = data + i;
      int *w = ws + i;
      int t0, t1, t2, t3, t4, t5, t6, t7, t10, t11, t12, t13, z5, z10, z11, z12, z13;
      if (in[8] == 0 && in[16] == 0 && in[24] == 0 && in[32] == 0 && in[40] == 0 && in[48] == 0 && in[56] == 0) {
         int dc = STBI__MB_DEQ(in[0], i);
         w[0] = w[8] = w[16] = w[24] = w[32] = w[40] = w[48] = w[56] = dc;
         continue;
      }
      t0 = STBI__MB_DEQ(in[0], i);
      t1 = STBI__MB_DEQ(in[16], i + 16);
      t2 = STBI__MB_DEQ(in[32], i + 32);
      t3 = STBI__MB_DEQ(in[48], i + 48);
      t10 = t0 + t2;
      t11 = t0 - t2;
      t13 = t1 + t3;
      t12 = STBI__MB_MUL(t1 - t3, 362) - t13;
      t0 = t10 + t13;
      t3 = t10 - t13;
      t1 = t11 + t12;
      t2 = t11 - t12;
      t4 = STBI__MB_DEQ(in[8], i + 8);
      t5 = STBI__MB_DEQ(in[24], i + 24);
      t6 = STBI__MB_DEQ(in[40], i + 40);
      t7 = STBI__MB_DEQ(in[56], i + 56);
      z13 = t6 + t5;
      z10 = t6 - t5;
      z11 = t4 + t7;
      z12 = t4 - t7;
      t7 = z11 + z13;
      t11 = STBI__MB_MUL(z11 - z13, 362);
      z5 = STBI__MB_MUL(z10 + z12, 473);
      t10 = STBI__MB_MUL(z12, 277) - z5;
      t12 = STBI__MB_MUL(z10, -669) + z5;
      t6 = t12 - t7;
      t5 = t11 - t6;
      t4 = t10 + t5;
      w[0]  = t0 + t7;
      w[56] = t0 - t7;
      w[8]  = t1 + t6;
      w[48] = t1 - t6;
      w[16] = t2 + t5;
      w[40] = t2 - t5;
      w[32] = t3 + t4;
      w[24] = t3 - t4;
   }
   for (i = 0; i < 8; ++i) {                        // rows (the result is 8x larger plus 2 fraction bits; +128 level shift)
      const int *w = ws + i * 8;
      stbi_uc *o = out + i * out_stride;
      int t0, t1, t2, t3, t4, t5, t6, t7, t10, t11, t12, t13, z5, z10, z11, z12, z13;
      if (w[1] == 0 && w[2] == 0 && w[3] == 0 && w[4] == 0 && w[5] == 0 && w[6] == 0 && w[7] == 0) {
         stbi_uc v = stbi__mb_clamp(((w[0] + 16) >> 5) + 128);
         o[0] = o[1] = o[2] = o[3] = o[4] = o[5] = o[6] = o[7] = v;
         continue;
      }
      t10 = w[0] + w[4];
      t11 = w[0] - w[4];
      t13 = w[2] + w[6];
      t12 = STBI__MB_MUL(w[2] - w[6], 362) - t13;
      t0 = t10 + t13;
      t3 = t10 - t13;
      t1 = t11 + t12;
      t2 = t11 - t12;
      z13 = w[5] + w[3];
      z10 = w[5] - w[3];
      z11 = w[1] + w[7];
      z12 = w[1] - w[7];
      t7 = z11 + z13;
      t11 = STBI__MB_MUL(z11 - z13, 362);
      z5 = STBI__MB_MUL(z10 + z12, 473);
      t10 = STBI__MB_MUL(z12, 277) - z5;
      t12 = STBI__MB_MUL(z10, -669) + z5;
      t6 = t12 - t7;
      t5 = t11 - t6;
      t4 = t10 + t5;
      o[0] = stbi__mb_clamp(((t0 + t7 + 16) >> 5) + 128);
      o[7] = stbi__mb_clamp(((t0 - t7 + 16) >> 5) + 128);
      o[1] = stbi__mb_clamp(((t1 + t6 + 16) >> 5) + 128);
      o[6] = stbi__mb_clamp(((t1 - t6 + 16) >> 5) + 128);
      o[2] = stbi__mb_clamp(((t2 + t5 + 16) >> 5) + 128);
      o[5] = stbi__mb_clamp(((t2 - t5 + 16) >> 5) + 128);
      o[4] = stbi__mb_clamp(((t3 + t4 + 16) >> 5) + 128);
      o[3] = stbi__mb_clamp(((t3 - t4 + 16) >> 5) + 128);
   }
}

static void stbi__setup_jpeg(stbi__jpeg *j)
{
   j->idct_block_kernel = stbi_mb_fast_idct ? stbi__mb_idct_fast : stbi__idct_block;
"@
$s = Edit-Exact $s $oldSetup $newSetup
'part 1 patched'
} else { 'part 1 already patched' }

# ---- part 2
if (-not $s.Contains('stbi__mb_idct_fast_dq')) {
  # 2a. AC decoding of progressive scans (refinement scans with the bit buffer in local variables)
  $src = Join-Path $PSScriptRoot 'MINIBROW-progac.txt'
  if (-not (Test-Path -LiteralPath $src)) { throw 'MINIBROW-progac.txt not found next to the script' }
  $progAc = L ([IO.File]::ReadAllText($src, $enc).TrimEnd("`r", "`n"))
  $a = $s.IndexOf('static int stbi__jpeg_decode_block_prog_ac(')
  $b = $s.IndexOf('// take a -128..127 value and stbi__clamp it and convert to 0..255')
  if ($a -lt 0 -or $b -lt $a) { throw 'stbi__jpeg_decode_block_prog_ac not found' }
  # the comment block just before the function stays; MINIBROW-progac.txt starts with its own comment and macros
  $s = $s.Substring(0, $a) + $progAc + $nl + $nl + $s.Substring($b)

  # 2b. a copy of the fast IDCT that dequantizes as it reads (same as stbi__jpeg_dequantize followed by stbi__mb_idct_fast)
  $a = $s.IndexOf('static void stbi__mb_idct_fast(stbi_uc *out, int out_stride, short data[64])')
  $b = $s.IndexOf('static void stbi__setup_jpeg(stbi__jpeg *j)')
  if ($a -lt 0 -or $b -lt $a) { throw 'stbi__mb_idct_fast not found' }
  $dq = $s.Substring($a, $b - $a).Replace('static void stbi__mb_idct_fast(stbi_uc *out, int out_stride, short data[64])', 'static void stbi__mb_idct_fast_dq(stbi_uc *out, int out_stride, short data[64], const stbi__uint16 *dq)')
  $n = ([regex]::Matches($dq, 'STBI__MB_DEQ\(')).Count
  if ($n -ne 9) { throw "expected 9 STBI__MB_DEQ uses in the fast IDCT, found $n" }
  $dq = $dq.Replace('STBI__MB_DEQ(', 'STBI__MB_DEQQ(')
  $dqHead = L ("// MiniBrow: progressive JPEG finish. stbi__jpeg_dequantize multiplied all 64 coefficients and wrote them back before the`n" +
               "// IDCT read them again. This copy dequantizes as it reads, only nonzero coefficients: (short)(c * q) is what`n" +
               "// stbi__jpeg_dequantize stores. A column whose AC coefficients dequantize to 0 takes the full path, which gives the same`n" +
               "// values as the DC-only shortcut.`n" +
               "#define STBI__MB_DEQQ(c, k)  ((c) ? (((int) (short) ((c) * dq[k]) * stbi__mb_aan[k] + 2048) >> 12) : 0)`n`n")
  $s = $s.Substring(0, $b) + $dqHead + $dq + $s.Substring($b)
  $oldFin = L ("               stbi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);`n" +
               "               z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);`n")
  $newFin = L ("               if (z->idct_block_kernel == stbi__mb_idct_fast) {`n" +
               "                  stbi__mb_idct_fast_dq(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);`n" +
               "               } else {`n" +
               "                  stbi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);`n" +
               "                  z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);`n" +
               "               }`n")
  $s = Edit-Exact $s $oldFin $newFin
  $s = Edit-Exact $s (L "static void stbi__jpeg_finish(stbi__jpeg *z)`n{") (L ("static void stbi__mb_idct_fast(stbi_uc *out, int out_stride, short data[64]);`n" +
       "static void stbi__mb_idct_fast_dq(stbi_uc *out, int out_stride, short data[64], const stbi__uint16 *dq);`n`n" +
       "static void stbi__jpeg_finish(stbi__jpeg *z)`n{"))
  'part 2 patched'
} else { 'part 2 already patched' }


# ---- part 4
if (-not $s.Contains('stbi__mb_idct_reduce')) {
  # 4a. the shift of the decoder (0 = full size, 1 = 1/2, 2 = 1/4, 3 = 1/8)
  $s = Edit-Exact $s (L "   stbi__uint16 dequant[4][64];`n") (L ("   stbi__uint16 dequant[4][64];`n" +
       "   int          mb_shift;       // MiniBrow: decode at 1/2^mb_shift of the size (stbi_mb_jpeg_shift)`n"))
  $s = Edit-Exact $s (L "   j->idct_block_kernel = stbi_mb_fast_idct ? stbi__mb_idct_fast : stbi__idct_block;`n") (L ("   j->idct_block_kernel = stbi_mb_fast_idct ? stbi__mb_idct_fast : stbi__idct_block;`n" +
       "   j->mb_shift = stbi_mb_jpeg_shift < 0 ? 0 : stbi_mb_jpeg_shift > 3 ? 3 : stbi_mb_jpeg_shift;`n"))

  # 4b. the reduced-size IDCT
  $red = L @'
// MiniBrow: JPEG decoded at 1/2, 1/4 or 1/8 of its size (stbi_mb_jpeg_shift = 1, 2, 3; MiniBrow sets it from the size the
// image is shown at). The pixels are the 8x8 block averaged over 2x2, 4x4 or 8x8 pixels, computed from the low-frequency
// coefficients only: the average of neighbouring samples of an 8-point inverse DCT basis function is a 4- or 2-point one
// times a constant (cos(u*pi/16) for pairs; for groups of four the averages are given below, frequency 2 averages to 0), so
// the reduced block is a small inverse DCT of weighted coefficients. The higher coefficients are left out (as in libjpeg's
// scaled decoding). A block with only DC gives the same value as the full-size IDCT gives to every pixel of it.
int stbi_mb_jpeg_shift = 0;
#define STBI__MB_RC(c, k)  (dq ? (int) (short) ((c) * dq[k]) : (int) (c))
static void stbi__mb_idct_reduce(stbi_uc *out, int out_stride, short data[64], const stbi__uint16 *dq, int shift)
{
   int t[16], i;
   if (shift >= 3) {
      int dc = STBI__MB_RC(data[0], 0);
      out[0] = stbi__mb_clamp(((dc + 4) >> 3) + 128);
      return;
   }
   if (shift == 1) {
      // 4x4: columns (vertical frequencies u = 0..3) then rows; constants * 4096:
      // 2896 = C0, 2676 = C0 cos(pi/4)... = 0.70711 * cos(2pi/16); 3711 / 1303 / 1537 / 3146 = cos(pi/8 or 3pi/8) * cos(pi/16 or 3pi/16)
      for (i = 0; i < 4; ++i) {
         int f0 = data[i] ? STBI__MB_RC(data[i], i) : 0, f1 = data[8 + i] ? STBI__MB_RC(data[8 + i], 8 + i) : 0;
         int f2 = data[16 + i] ? STBI__MB_RC(data[16 + i], 16 + i) : 0, f3 = data[24 + i] ? STBI__MB_RC(data[24 + i], 24 + i) : 0;
         int e0 = f0 * 2896 + f2 * 2676, e1 = f0 * 2896 - f2 * 2676;
         int o0 = f1 * 3711 + f3 * 1303, o1 = f1 * 1537 - f3 * 3146;
         t[i]      = (e0 + o0 + 2048) >> 12;
         t[12 + i] = (e0 - o0 + 2048) >> 12;
         t[4 + i]  = (e1 + o1 + 2048) >> 12;
         t[8 + i]  = (e1 - o1 + 2048) >> 12;
      }
      for (i = 0; i < 4; ++i) {
         const int *w = t + i * 4;
         stbi_uc *o = out + i * out_stride;
         int e0 = w[0] * 2896 + w[2] * 2676, e1 = w[0] * 2896 - w[2] * 2676;
         int o0 = w[1] * 3711 + w[3] * 1303, o1 = w[1] * 1537 - w[3] * 3146;
         o[0] = stbi__mb_clamp(((e0 + o0 + 8192) >> 14) + 128);   // / 4096, then 1/2 per direction
         o[3] = stbi__mb_clamp(((e0 - o0 + 8192) >> 14) + 128);
         o[1] = stbi__mb_clamp(((e1 + o1 + 8192) >> 14) + 128);
         o[2] = stbi__mb_clamp(((e1 - o1 + 8192) >> 14) + 128);
      }
      return;
   }
   // 2x2: frequencies 0, 1 and 3 in each direction; averages of the basis over 4 samples * 4096:
   // 2624 (u = 1, 0.640729) and -922 (u = 3, -0.224994), the second output has the opposite sign
   {
      static const int idx[3] = { 0, 1, 3 };
      for (i = 0; i < 3; ++i) {
         int v = idx[i];
         int f0 = data[v] ? STBI__MB_RC(data[v], v) : 0, f1 = data[8 + v] ? STBI__MB_RC(data[8 + v], 8 + v) : 0;
         int f3 = data[24 + v] ? STBI__MB_RC(data[24 + v], 24 + v) : 0;
         int e = f0 * 2896, o = f1 * 2624 - f3 * 922;
         t[i]     = (e + o + 2048) >> 12;
         t[3 + i] = (e - o + 2048) >> 12;
      }
      for (i = 0; i < 2; ++i) {
         const int *w = t + i * 3;
         int e = w[0] * 2896, o = w[1] * 2624 - w[2] * 922;
         out[i * out_stride]     = stbi__mb_clamp(((e + o + 8192) >> 14) + 128);
         out[i * out_stride + 1] = stbi__mb_clamp(((e - o + 8192) >> 14) + 128);
      }
   }
}

'@
  $b = $s.IndexOf('static void stbi__setup_jpeg(stbi__jpeg *j)')
  if ($b -lt 0) { throw 'stbi__setup_jpeg not found' }
  $s = $s.Substring(0, $b) + $red + $s.Substring($b)

  # 4c. component planes at the reduced size
  # each component is reduced by mb_cs; as in libjpeg 6b, a subsampled component is reduced less while both of its
  # ratios to the largest factors stay divisible by 2 (4:2:0 at 1/4: chroma at 1/2), so its colours keep their detail
  $s = Edit-Exact $s (L "      int x,y,w2,h2;`n") (L ("      int x,y,w2,h2;`n" +
       "      int mb_cs;   // MiniBrow: this plane is decoded at 1/2^mb_cs (mb_cs <= mb_shift)`n"))
  $s = Edit-Exact $s (L "      z->img_comp[i].raw_data = stbi__malloc_mad2(z->img_comp[i].w2, z->img_comp[i].h2, 15);`n") (L ("      {   // MiniBrow: the reduction of this plane (h_max / h and v_max / v are integers, checked above)`n" +
       "         int rh = h_max / z->img_comp[i].h, rv = v_max / z->img_comp[i].v, k2 = 0;`n" +
       "         while (k2 < z->mb_shift && rh % (2 << k2) == 0 && rv % (2 << k2) == 0) ++k2;`n" +
       "         z->img_comp[i].mb_cs = z->mb_shift - k2;`n" +
       "      }`n" +
       "      z->img_comp[i].raw_data = stbi__malloc_mad2(z->img_comp[i].w2 >> z->img_comp[i].mb_cs, z->img_comp[i].h2 >> z->img_comp[i].mb_cs, 15);   // MiniBrow: reduced size`n"))

  # 4d. IDCT callers write reduced blocks
  $o = "z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);"
  $cnt = ([regex]::Matches($s, [regex]::Escape($o))).Count
  if ($cnt -ne 2) { throw "expected 2 block IDCT calls (baseline, progressive), found $cnt" }
  $s = $s.Replace($o, "if (z->img_comp[n].mb_cs) stbi__mb_idct_reduce(z->img_comp[n].data+(z->img_comp[n].w2>>z->img_comp[n].mb_cs)*j*(8>>z->img_comp[n].mb_cs)+i*(8>>z->img_comp[n].mb_cs), z->img_comp[n].w2>>z->img_comp[n].mb_cs, data, NULL, z->img_comp[n].mb_cs); else z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data);")
  $s = Edit-Exact $s "z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data);" "if (z->img_comp[n].mb_cs) stbi__mb_idct_reduce(z->img_comp[n].data+(z->img_comp[n].w2>>z->img_comp[n].mb_cs)*(y2>>z->img_comp[n].mb_cs)+(x2>>z->img_comp[n].mb_cs), z->img_comp[n].w2>>z->img_comp[n].mb_cs, data, NULL, z->img_comp[n].mb_cs); else z->idct_block_kernel(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data);"
  # the progressive finish: raw coefficients, dequantized in the reduced IDCT
  $s = Edit-Exact $s (L "               if (z->idct_block_kernel == stbi__mb_idct_fast) {`n                  stbi__mb_idct_fast_dq(") (L "               if (z->img_comp[n].mb_cs) {   // MiniBrow: reduced size`n                  stbi__mb_idct_reduce(z->img_comp[n].data+(z->img_comp[n].w2>>z->img_comp[n].mb_cs)*j*(8>>z->img_comp[n].mb_cs)+i*(8>>z->img_comp[n].mb_cs), z->img_comp[n].w2>>z->img_comp[n].mb_cs, data, z->dequant[z->img_comp[n].tq], z->img_comp[n].mb_cs);`n               } else if (z->idct_block_kernel == stbi__mb_idct_fast) {`n                  stbi__mb_idct_fast_dq(")
  # declaration before the baseline callers (stbi__parse_entropy_coded_data comes before the definition)
  $s = Edit-Exact $s (L "static int stbi__parse_entropy_coded_data(stbi__jpeg *z)`n{") (L "static void stbi__mb_idct_reduce(stbi_uc *out, int out_stride, short data[64], const stbi__uint16 *dq, int shift);`n`nstatic int stbi__parse_entropy_coded_data(stbi__jpeg *z)`n{")

  # 4e. after decoding, the image and the planes have the reduced size (rounded up). A plane reduced 2^(mb_shift - mb_cs)
  # times less gets factors that much larger, so the resampler sees the ratio that is left (1 for 4:2:0 at 1/2 or less):
  # ceil(ceil(a/h)/s) == ceil(ceil(a/s)/h) keeps the plane sizes and the ratios consistent
  $s = Edit-Exact $s (L "   if (!stbi__decode_jpeg_image(z)) { stbi__cleanup_jpeg(z); return NULL; }`n") (L ("   if (!stbi__decode_jpeg_image(z)) { stbi__cleanup_jpeg(z); return NULL; }`n" +
       "   if (z->mb_shift) {   // MiniBrow: the planes were decoded at 1/2^mb_cs, the image is 1/2^mb_shift`n" +
       "      int k2, sh = z->mb_shift, add = (1 << sh) - 1;`n" +
       "      z->s->img_x = (z->s->img_x + add) >> sh;`n" +
       "      z->s->img_y = (z->s->img_y + add) >> sh;`n" +
       "      for (k2 = 0; k2 < z->s->img_n; ++k2) {`n" +
       "         int cs = z->img_comp[k2].mb_cs, a2 = (1 << cs) - 1;`n" +
       "         z->img_comp[k2].x = (z->img_comp[k2].x + a2) >> cs;`n" +
       "         z->img_comp[k2].y = (z->img_comp[k2].y + a2) >> cs;`n" +
       "         z->img_comp[k2].w2 >>= cs;`n" +
       "         z->img_comp[k2].h2 >>= cs;`n" +
       "         z->img_comp[k2].h <<= sh - cs;   // h_max / h is the ratio left for the resampler`n" +
       "         z->img_comp[k2].v <<= sh - cs;`n" +
       "      }`n" +
       "   }`n"))
  'part 4 patched'
} else { 'part 4 already patched' }

[IO.File]::WriteAllText($File, $s, $enc)
