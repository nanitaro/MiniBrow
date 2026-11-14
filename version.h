/*
 * version.h - MiniBrow の版番号 (ここだけで決める)
 *
 *  - minibrow.c: UserAgent (USER_AGENT / JS_USER_AGENT) とスタートページの見出し
 *  - minibrow.rc: exe のプロパティの版情報 (FILEVERSION と文字列)
 *  - mkrelease.ps1: リリースの zip の名前 (MiniBrow-<MB_VERSION>.zip)
 *
 * 変えるときは 3 つとも合わせる (mkrelease.ps1 が MB_VER_MAJOR.MB_VER_MINOR と MB_VERSION の一致を確かめる)
 */
#ifndef VERSION_H
#define VERSION_H

#define MB_VER_MAJOR    0
#define MB_VER_MINOR    2
#define MB_VERSION      "0.2"

#endif
