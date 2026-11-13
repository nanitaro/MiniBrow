/*
 * w95_xs.h - MiniBrow 用の XS のプラットフォーム定義 (Windows 95 / Open Watcom C)
 *
 * XS のソースは XSPLATFORM でこのファイルを読み込む (w95pre.h で指定)。
 * XS 付属の Windows 版 (win_xs.c) は Win95 に無い HWND_MESSAGE などを使うので使わない。
 */
#ifndef __W95_XS__
#define __W95_XS__

#undef mxWindows
#define mxWindows 1
#undef mxLittleEndian
#define mxLittleEndian 1

#define WIN32_LEAN_AND_MEAN
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <windows.h>
#include <sys/timeb.h>

/* 数学関数は fdlibm を使う (Watcom に無いものを補い、CPU による結果の違いも無くす) */
#include <fdlibm.h>

#define mxExport extern
#define mxImport extern

#define mxMachinePlatform \
	int promiseJobs; \
	void* host;

#define mxUseDefaultMachinePlatform 1
#define mxUseDefaultBuildKeys 1
#define mxUseDefaultChunkAllocation 1
#define mxUseDefaultSlotAllocation 1
#define mxUseDefaultFindModule 1
#define mxUseDefaultLoadModule 1
#define mxUseDefaultParseScript 1
#define mxUseDefaultSharedChunks 1

/* 言語と大きさの設定 */
#define mxCESU8 1                       /* 文字列の長さをブラウザと同じ UTF-16 単位にする */
#define mxCanonicalNaN 1
#define mxAliasInstance 0
#define mxHostFunctionPrimitive 0
#define mxKeysGarbageCollection 1
#define mxRegExpUnicodePropertyEscapes 0 /* 表が大きいので入れない */
#define mxStringNormalize 0
#define mxFloat16 0
#define mxMetering 1                    /* 暴走したスクリプトを止める */
#define mxW95 1                         /* 共有メモリのスレッドを使わない (Win95 に条件変数が無い) */

#ifndef c_llround
	#define c_llround(x) ((long long)round(x))
#endif

#define C_PATH_MAX MAX_PATH

#define mxFallThrough (void)0

/* XS が識別子に使っている語のうち Watcom では予約語のもの */
#define _export xs__export
#define _import xs__import

#define C_M_E        2.718281828459045235360
#define C_M_LN10     2.302585092994045684018
#define C_M_LN2      0.693147180559945309417
#define C_M_LOG10E   0.434294481903251827651
#define C_M_LOG2E    1.442695040888963407360
#define C_M_PI       3.141592653589793238463
#define C_M_SQRT1_2  0.707106781186547524401
#define C_M_SQRT2    1.414213562373095048802

/* Watcom のヘッダに無い Win32 の定義 */
#define _Out_
void *NtCurrentTeb(void);
#pragma aux NtCurrentTeb = "mov eax, fs:[18h]" value [eax];

/* gettimeofday は xsPlatforms.c (mxWindows の部分) が _ftime で作る */
typedef struct { long tv_sec; long tv_usec; } c_timeval;
struct c_timezone { int tz_minuteswest; int tz_dsttime; };
extern int c_gettimeofday(c_timeval *tp, struct c_timezone *tzp);
#define c_gettimeofday c_gettimeofday

/* XS のメモリ確保は上限付きの関数を通す (w95_xs.c)。上限を超えると NULL を返す */
extern void *w95_malloc(size_t n);
extern void *w95_calloc(size_t a, size_t b);
extern void *w95_realloc(void *p, size_t n);
extern void w95_free(void *p);
extern long  g_xsMemLimit;              /* 上限 (バイト。0 = 無制限) */
extern long  g_xsMemUsed;               /* 使用中の量 */
#define c_malloc w95_malloc
#define c_calloc w95_calloc
#define c_realloc w95_realloc
#define c_free w95_free
#define c_malloc_uint32 w95_malloc
#define c_free_uint32 w95_free

/* Math.random: 起動ごとに違う並びになるよう時刻で種をまく 32 ビットの乱数 (w95_xs.c)。
   C の rand は種をまかないと毎回同じで、15 ビットしかない */
extern unsigned long w95_rand(void);
#define c_rand w95_rand
#define C_RAND_MAX 0xFFFFFFFFUL

/* Watcom の sqrt は負の数に 0 を返す (JavaScript では NaN) */
extern double w95_sqrt(double x);
#define c_sqrt w95_sqrt
/* log2 は fdlibm に無く Watcom のものになるが、負の数に -Infinity を返す */
extern double w95_log2(double x);
#define c_log2 w95_log2

#endif /* __W95_XS__ */
