/*
 * mkprel.c - 前処理部分 (prelude.inc) を XS で解析し、バイトコードを C の配列 (prelude.h) にして書き出す
 *
 * ビルドのときだけ動かす道具。実行時の解析を省くため (Pentium 100MHz で約 0.7 秒かかる)。
 * makefile が prelude.inc か xs.lib の変更で作り直す。
 */
#include "xsAll.h"
#include "xs.h"
#include <stdio.h>

static const char g_prelude[] =
#include "prelude.inc"

static void WriteBytes(FILE *f, const char *name, const txS1 *p, long n)
{
    long i;

    fprintf(f, "static const unsigned char %s[] = {", name);
    for (i = 0; i < n; i++) {
        fprintf(f, "%s%u,", i % 32 == 0 ? "\n" : "", (unsigned)(unsigned char)p[i]);
    }
    fprintf(f, "\n};\n");
}

int main(int argc, char **argv)
{
    static xsCreation creation = {
        64 * 1024, 256 * 1024, 4 * 1024, 32 * 1024, 32 * 1024, 512, 256, 397, 61, 64 * 1024, 397, 0, 0
    };
    xsMachine *machine;
    txScript  *script = NULL;
    FILE      *f;

    if (argc < 2) {
        return 2;
    }
    machine = xsCreateMachine(&creation, "mkprel", NULL);
    if (machine == NULL) {
        fprintf(stderr, "mkprel: cannot create the machine\n");
        return 1;
    }
    xsBeginHost(machine);
    {
        txStringCStream stream;
        stream.buffer = (char *)g_prelude;
        stream.offset = 0;
        stream.size   = (txSize)strlen(g_prelude);
        script = fxParseScript(the, &stream, fxStringCGetter, mxProgramFlag);
    }
    xsEndHost(machine);
    if (script == NULL) {
        fprintf(stderr, "mkprel: the prelude has a syntax error\n");
        return 1;
    }
    if (script->hostsSize != 0) {
        fprintf(stderr, "mkprel: host functions are not supported\n");
        return 1;
    }
    f = fopen(argv[1], "w");
    if (f == NULL) {
        fprintf(stderr, "mkprel: cannot write %s\n", argv[1]);
        return 1;
    }
    fprintf(f, "/* mkprel.exe が prelude.inc から作るファイル。手で変えない */\n");
    WriteBytes(f, "g_preSym", script->symbolsBuffer, script->symbolsSize);
    WriteBytes(f, "g_preCode", script->codeBuffer, script->codeSize);
    fprintf(f, "static const unsigned char g_preVer[4] = { %d, %d, %d, %d };\n",
            (int)script->version[0], (int)script->version[1], (int)script->version[2], (int)script->version[3]);
    fclose(f);
    return 0;
}
