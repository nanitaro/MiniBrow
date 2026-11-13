/*
 * stbimg.c - stb_image (画像デコーダ) の実体を置くだけのファイル
 *
 *  - JPEG / PNG / GIF / BMP だけを有効にする
 *  - メモリ上のデータからのみ読む (STBI_NO_STDIO)
 *  - 386 / Win95 で動かすため SIMD は使わない
 */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_BMP
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb\stb_image.h"
