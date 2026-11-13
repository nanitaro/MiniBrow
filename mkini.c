/*
 * mkini.c - minibrow.ini を C の配列 (defini.h) にして書き出す
 *
 * ビルドのときだけ動かす道具。minibrow.ini が無いときに exe が同じ内容で作るため。
 * Proxy= / JSLog= / UserAgent= は、作業している環境の値が配布物に入らないように空にする。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const g_blank[] = { "Proxy=", "JSLog=", "UserAgent=" };

/* 行 p (長さ n) を空にする項目か。そうなら key の長さを返す */
static int BlankLen(const char *p, long n)
{
    int i;
    for (i = 0; i < (int)(sizeof(g_blank) / sizeof(g_blank[0])); i++) {
        int k = (int)strlen(g_blank[i]);
        if (n >= k && memcmp(p, g_blank[i], k) == 0) {
            return k;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    char *buf;
    long  len, i = 0, col = 0;

    if (argc < 3 || (in = fopen(argv[1], "rb")) == NULL) {
        fprintf(stderr, "mkini: cannot read the ini file\n");
        return 1;
    }
    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);
    buf = malloc(len + 1);
    if (buf == NULL || fread(buf, 1, len, in) != (size_t)len) {
        fprintf(stderr, "mkini: cannot read the ini file\n");
        return 1;
    }
    fclose(in);
    if ((out = fopen(argv[2], "w")) == NULL) {
        fprintf(stderr, "mkini: cannot write %s\n", argv[2]);
        return 1;
    }
    fprintf(out, "/* mkini.exe が minibrow.ini から作るファイル。手で変えない */\n");
    fprintf(out, "static const unsigned char g_defIni[] = {");
    while (i < len) {
        long e = i, k, skip;
        while (e < len && buf[e] != '\r' && buf[e] != '\n') {
            e++;
        }
        skip = BlankLen(buf + i, e - i);
        for (k = i; k < e; k++) {
            if (skip == 0 || k - i < skip) {
                fprintf(out, "%s%u,", col++ % 32 == 0 ? "\n" : "", (unsigned)(unsigned char)buf[k]);
            }
        }
        i = e;
        while (i < len && (buf[i] == '\r' || buf[i] == '\n')) {
            fprintf(out, "%s%u,", col++ % 32 == 0 ? "\n" : "", (unsigned)(unsigned char)buf[i]);
            i++;
        }
    }
    fprintf(out, "\n};\n");
    fclose(out);
    free(buf);
    return 0;
}
