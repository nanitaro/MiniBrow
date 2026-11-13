/*
 * imgext.c - WebP (simplewebp) と SVG (nanosvg) の実体を置くだけのファイル
 *
 *  - どちらもメモリ上のデータからのみ読む
 *  - nanosvg は浮動小数点を使う (FPU の無い 386/486SX では遅い)
 */
#define SIMPLEWEBP_IMPLEMENTATION
#define SIMPLEWEBP_DISABLE_STDIO
#include "imglib\simplewebp.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

/*
 * Open Watcom の実行時ライブラリには C99 の float 版数学関数の実体が無いので、
 * nanosvg が使うものだけ double 版で用意する
 */
float sinf(float x)             { return (float)sin(x); }
float cosf(float x)             { return (float)cos(x); }
float tanf(float x)             { return (float)tan(x); }
float acosf(float x)            { return (float)acos(x); }
float atan2f(float y, float x)  { return (float)atan2(y, x); }
float sqrtf(float x)            { return (float)sqrt(x); }
float fabsf(float x)            { return (float)fabs(x); }
float floorf(float x)           { return (float)floor(x); }
float ceilf(float x)            { return (float)ceil(x); }
float fmodf(float x, float y)   { return (float)fmod(x, y); }
float roundf(float x)           { return (float)(x >= 0.0f ? floor(x + 0.5) : ceil(x - 0.5)); }

#define NANOSVG_IMPLEMENTATION
#include "imglib\nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "imglib\nanosvgrast.h"
