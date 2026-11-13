# Open Watcom wmake makefile - Win32 (GUI)

# USE_WOLFSSL=1 (wmake USE_WOLFSSL=1): TLS を wolfSSL で行う研究用のビルド (minibrow_wolf.exe)。
# wolfSSL は GPLv3 なので、このビルドの exe は配布しない。既定は BearSSL の配布用のビルド (minibrow.exe)
!ifdef USE_WOLFSSL
TARGET  = minibrow_wolf.exe
!else
TARGET  = minibrow.exe
!endif

CC      = wcc386

# minibrow とライブラリ共通の最適化 (ここを変えれば全部に効く)
#   -3r     : 386 以降で動く命令だけを使う (Windows 95 機向け)
#   -ot     : 実行速度を優先する
#   -s      : スタック検査をしない
# 実測 (JPEG/WebP デコード, SHA-256, P-256) では -ot -s が最も速かった。
# -ox は JPEG が、-ox -oh は JPEG と WebP が -os (サイズ優先) より遅くなった
# (展開でコードが膨らむため。キャッシュの小さい古い CPU ではさらに不利)。
# オプションを変えたら wmake clean してから作り直すこと (wmake はフラグの変化を検出しない)
MBOPT   = -3r -ot -s

# minibrow: -bm = マルチスレッド
MBFLAGS = -q -bt=nt $(MBOPT) -bm -w4 -zk0 -i=$(%WATCOM)\h\nt -ibearssl\inc
MBLINK  = system nt_win option quiet runtime windows=4.0 library { bearssl xs wsock32 comdlg32 }

all: $(TARGET) .SYMBOLIC

# BearSSL (HTTPS 用) のオブジェクト一覧とビルド規則
!include bearssl.mak

# Moddable XS (JavaScript 用) のオブジェクト一覧とビルド規則
!include xs.mak

!ifdef USE_WOLFSSL
# wolfSSL (研究用のビルドだけ。ソースは wolfssl\、設定は wolfcfg\user_settings.h)
!include wolfssl.mak
!endif

minibrow.exe: minibrow.obj mbrsa.obj css.obj cookie.obj js.obj stbimg.obj imgext.obj minibrow.res bearssl.lib xs.lib
	wlink $(MBLINK) option resource=minibrow.res name $@ file { minibrow.obj mbrsa.obj css.obj cookie.obj js.obj stbimg.obj imgext.obj }

# 研究用のビルドは BearSSL を使わない (TLS・証明書・乱数・ハッシュはすべて wolfSSL / wolfCrypt)
MBLINKW = system nt_win option quiet runtime windows=4.0 library { wolfssl xs wsock32 comdlg32 }

minibrow_wolf.exe: minibrow_w.obj wolftls.obj css.obj cookie.obj js.obj stbimg.obj imgext.obj minibrow.res xs.lib wolfssl.lib
	wlink $(MBLINKW) option resource=minibrow.res name $@ file { minibrow_w.obj wolftls.obj css.obj cookie.obj js.obj stbimg.obj imgext.obj }

# exe のプロパティの版情報 (版番号は version.h)
minibrow.res: minibrow.rc version.h
	wrc -q -r -bt=nt -fo=$@ minibrow.rc

# minibrow.ini の既定の内容 (ビルドのときに作って minibrow.c に取り込む)
defini.h: mkini.exe minibrow.ini
	mkini.exe minibrow.ini defini.h

mkini.exe: mkini.c
	$(CC) $(MBFLAGS) -fo=mkini.obj mkini.c
	wlink system nt option quiet name mkini.exe file { mkini.obj }

minibrow.obj: minibrow.c defini.h .AUTODEPEND
	$(CC) $(MBFLAGS) -fo=$@ minibrow.c

# 研究用のビルド: 本体は別の名前のオブジェクトにする (配布用のビルドと混ざらないように)
minibrow_w.obj: minibrow.c defini.h .AUTODEPEND
	$(CC) $(MBFLAGS) -DUSE_WOLFSSL -fo=$@ minibrow.c

# -wcd=202: wolfSSL のヘッダにある使われない static 関数の警告だけを止める
wolftls.obj: wolftls.c wolftls.h wolfcfg\user_settings.h
	$(CC) -q -bt=nt $(MBOPT) -bm -w4 -wcd=202 -za99 -zk0 -DWOLFSSL_USER_SETTINGS -iwolfcfg -iwolfssl -fo=$@ wolftls.c

# RSA の公開鍵の計算 (配布用のビルドだけ。BearSSL の i31 の代わり。掛け算と足し込みは x86 の命令)
mbrsa.obj: mbrsa.c mbrsa.h
	$(CC) $(MBFLAGS) -fo=$@ mbrsa.c

# スタイルシート (CSS) の解析と適用
css.obj: css.c .AUTODEPEND
	$(CC) $(MBFLAGS) -fo=$@ css.c

# Cookie の保管
cookie.obj: cookie.c .AUTODEPEND
	$(CC) $(MBFLAGS) -fo=$@ cookie.c

# JavaScript (XS の実行環境と window / document など)
# 前処理部分のバイトコード (ビルドのときに作って js.c に取り込む)
prelude.h: mkprel.exe prelude.inc
	mkprel.exe prelude.h

mkprel.exe: mkprel.c prelude.inc xs.lib xs\w95\w95_xs.h
	$(CC) $(MBFLAGS) -za99 -fi=xs\w95\w95pre.h -ixs\w95 -ixs\includes -ixs\platforms -ixs\sources -ixs\fdlibm -fo=mkprel.obj mkprel.c
	wlink system nt option quiet library { xs } name mkprel.exe file { mkprel.obj }

js.obj: js.c js.h prelude.h xs\w95\w95_xs.h
	$(CC) $(MBFLAGS) -za99 -fi=xs\w95\w95pre.h -ixs\w95 -ixs\includes -ixs\platforms -ixs\sources -ixs\fdlibm -fo=$@ js.c

# 画像デコーダ (stb_image, 外部コードなので警告は出さない)
stbimg.obj: stbimg.c stb\stb_image.h
	$(CC) -q -bt=nt $(MBOPT) -oe=40 -bm -w0 -za99 -zk0 -DNDEBUG -fo=$@ stbimg.c

# WebP (simplewebp) と SVG (nanosvg) のデコーダ (外部コード)
imgext.obj: imgext.c imglib\simplewebp.h imglib\nanosvg.h imglib\nanosvgrast.h
	$(CC) -q -bt=nt $(MBOPT) -oe=40 -bm -w0 -za99 -zk0 -DNDEBUG -fo=$@ imgext.c

# ビルドして実行
run: all exec .SYMBOLIC

# ビルドせずに exe をそのまま実行 (GUI はウィンドウを閉じると終わる)
exec: .SYMBOLIC
	.\$(TARGET)

clean: .SYMBOLIC
	@if exist defini.h del defini.h
	@if exist prelude.h del prelude.h
	@if exist *.obj del *.obj
	@if exist *.exe del *.exe
	@if exist *.err del *.err
	@if exist *.map del *.map
	@if exist *.res del *.res
	@if exist *.lib del *.lib
	@if exist bobj\*.obj del bobj\*.obj
	@if exist bobj rmdir bobj
	@if exist xobj\*.obj del xobj\*.obj
	@if exist xobj rmdir xobj
	@if exist wobj\*.obj del wobj\*.obj
	@if exist wobj rmdir wobj
