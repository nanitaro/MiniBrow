/*
 * w95_xs.c - MiniBrow 用の XS のプラットフォーム関数 (Windows 95 / Open Watcom C)
 */
#include "xsAll.h"

/* ---------------------------------------------------------------- メモリ */

long g_xsMemLimit;
long g_xsMemUsed;

/* 確保した大きさを先頭 8 バイトに置いて、使用量を数える */
void *w95_malloc(size_t n)
{
	size_t *p;
	if (g_xsMemLimit > 0 && g_xsMemUsed + (long)n > g_xsMemLimit)
		return NULL;
	p = (malloc)(n + 8);
	if (p == NULL)
		return NULL;
	p[0] = n;
	g_xsMemUsed += (long)n;
	return (char *)p + 8;
}

void *w95_calloc(size_t a, size_t b)
{
	void *p = w95_malloc(a * b);
	if (p)
		memset(p, 0, a * b);
	return p;
}

void w95_free(void *q)
{
	size_t *p;
	if (q == NULL)
		return;
	p = (size_t *)((char *)q - 8);
	g_xsMemUsed -= (long)p[0];
	(free)(p);
}

void *w95_realloc(void *q, size_t n)
{
	size_t *p, old;
	if (q == NULL)
		return w95_malloc(n);
	p = (size_t *)((char *)q - 8);
	old = p[0];
	if (g_xsMemLimit > 0 && n > old && g_xsMemUsed + (long)(n - old) > g_xsMemLimit)
		return NULL;
	p = (realloc)(p, n + 8);
	if (p == NULL)
		return NULL;
	p[0] = n;
	g_xsMemUsed += (long)n - (long)old;
	return (char *)p + 8;
}

/* ---------------------------------------------------------------- 実行の制御 */

/* Promise の後続処理は、呼び出し元 (js.c) が実行の区切りでまとめて走らせる */
void fxQueuePromiseJobs(txMachine* the)
{
	the->promiseJobs = 1;
}

/* 時間切れ・メモリ不足などで止める。プロセスは終わらせず、ホストの呼び出し位置へ戻る */
void fxAbort(txMachine* the, int status)
{
	if (status == XS_UNHANDLED_REJECTION_EXIT) {
		mxException = mxUndefined;      /* ブラウザと同じく、受け取られなかった Promise の失敗では止めない */
		return;
	}
	if (the->exitStatus)
		return;
	the->exitStatus = status;
	fxExitToHost(the);
}

/* Math.random 用の乱数 (xorshift32)。最初に時刻と性能カウンタで種をまく */
unsigned long w95_rand(void)
{
	static unsigned long s;
	if (s == 0) {
		LARGE_INTEGER c;
		if (!QueryPerformanceCounter(&c))
			c.LowPart = 0;
		s = (GetTickCount() * 2654435761UL) ^ c.LowPart ^ (unsigned long)GetCurrentProcessId();
		if (s == 0)
			s = 1;
	}
	s ^= s << 13;
	s ^= s >> 17;
	s ^= s << 5;
	return s & 0xFFFFFFFFUL;
}

double w95_sqrt(double x)
{
	if (isnan(x) || x < 0)
		return NAN;
	return (sqrt)(x);
}

double w95_log2(double x)
{
	if (isnan(x) || x < 0)
		return NAN;
	return c_log(x) / 0.69314718055994530942;
}

/* MSVC の組み込み関数 (Math.clz32 で使う) */
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask)
{
	unsigned long i = 31;
	if (mask == 0)
		return 0;
	while (!(mask & 0x80000000UL)) {
		mask <<= 1;
		i--;
	}
	*index = i;
	return 1;
}
